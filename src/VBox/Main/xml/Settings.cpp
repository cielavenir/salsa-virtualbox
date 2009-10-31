/** @file
 * Settings File Manipulation API.
 */

/*
 * Copyright (C) 2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/lock.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/globals.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlsave.h>
#include <libxml/uri.h>

#include <libxml/xmlschemas.h>

#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

#include <list>

// #include <string.h>

#include "VBox/settings.h"

#include "Logging.h"

namespace settings
{

// Helpers
////////////////////////////////////////////////////////////////////////////////

inline int sFromHex (char aChar)
{
    if (aChar >= '0' && aChar <= '9')
        return aChar - '0';
    if (aChar >= 'A' && aChar <= 'F')
        return aChar - 'A' + 0xA;
    if (aChar >= 'a' && aChar <= 'f')
        return aChar - 'a' + 0xA;

    throw ENoConversion(com::Utf8StrFmt("'%c' (0x%02X) is not hex", aChar, aChar));
}

inline char sToHex (int aDigit)
{
    return (aDigit < 0xA) ? aDigit + '0' : aDigit - 0xA + 'A';
}

static char *duplicate_chars (const char *that)
{
    char *result = NULL;
    if (that != NULL)
    {
        size_t len = strlen (that) + 1;
        result = new char [len];
        if (result != NULL)
            memcpy (result, that, len);
    }
    return result;
}

//////////////////////////////////////////////////////////////////////////////
// string -> type conversions
//////////////////////////////////////////////////////////////////////////////

uint64_t FromStringInteger (const char *aValue, bool aSigned,
                            int aBits, uint64_t aMin, uint64_t aMax)
{
    if (aValue == NULL)
        throw ENoValue();

    switch (aBits)
    {
        case 8:
        case 16:
        case 32:
        case 64:
            break;
        default:
            throw xml::ENotImplemented (RT_SRC_POS);
    }

    if (aSigned)
    {
        int64_t result;
        int vrc = RTStrToInt64Full (aValue, 0, &result);
        if (RT_SUCCESS (vrc))
        {
            if (result >= (int64_t) aMin && result <= (int64_t) aMax)
                return (uint64_t) result;
        }
    }
    else
    {
        uint64_t result;
        int vrc = RTStrToUInt64Full (aValue, 0, &result);
        if (RT_SUCCESS (vrc))
        {
            if (result >= aMin && result <= aMax)
                return result;
        }
    }

    throw ENoConversion(com::Utf8StrFmt("'%s' is not integer", aValue));
}

template<> bool FromString <bool> (const char *aValue)
{
    if (aValue == NULL)
        throw ENoValue();

    if (strcmp (aValue, "true") == 0 ||
        strcmp (aValue, "1") == 0)
        /* This contradicts the XML Schema's interpretation of boolean: */
        //strcmp (aValue, "yes") == 0 ||
        //strcmp (aValue, "on") == 0)
        return true;
    else if (strcmp (aValue, "false") == 0 ||
             strcmp (aValue, "0") == 0)
            /* This contradicts the XML Schema's interpretation of boolean: */
            //strcmp (aValue, "no") == 0 ||
            //strcmp (aValue, "off") == 0)
        return false;

    throw ENoConversion(com::Utf8StrFmt("'%s' is not bool", aValue));
}

template<> RTTIMESPEC FromString <RTTIMESPEC> (const char *aValue)
{
    if (aValue == NULL)
        throw ENoValue();

    /* Parse ISO date (xsd:dateTime). The format is:
     * '-'? yyyy '-' mm '-' dd 'T' hh ':' mm ':' ss ('.' s+)? (zzzzzz)?
     * where zzzzzz is: (('+' | '-') hh ':' mm) | 'Z' */
    uint32_t yyyy = 0;
    uint16_t mm = 0, dd = 0, hh = 0, mi = 0, ss = 0;
    char buf [256];
    if (strlen (aValue) > RT_ELEMENTS (buf) - 1 ||
        sscanf (aValue, "%d-%hu-%huT%hu:%hu:%hu%s",
                &yyyy, &mm, &dd, &hh, &mi, &ss, buf) == 7)
    {
        /* currently, we accept only the UTC timezone ('Z'),
         * ignoring fractional seconds, if present */
        if (buf [0] == 'Z' ||
            (buf [0] == '.' && buf [strlen (buf) - 1] == 'Z'))
        {
            RTTIME time = { yyyy, (uint8_t) mm, 0, 0, (uint8_t) dd,
                            (uint8_t) hh, (uint8_t) mi, (uint8_t) ss, 0,
                            RTTIME_FLAGS_TYPE_UTC };
            if (RTTimeNormalize (&time))
            {
                RTTIMESPEC timeSpec;
                if (RTTimeImplode (&timeSpec, &time))
                    return timeSpec;
            }
        }
        else
            throw ENoConversion(com::Utf8StrFmt("'%s' is not UTC date", aValue));
    }

    throw ENoConversion(com::Utf8StrFmt("'%s' is not ISO date", aValue));
}

stdx::char_auto_ptr FromString (const char *aValue, size_t *aLen)
{
    if (aValue == NULL)
        throw ENoValue();

    /* each two chars produce one byte */
    size_t len = strlen (aValue) / 2;

    /* therefore, the original length must be even */
    if (len % 2 != 0)
        throw ENoConversion(com::Utf8StrFmt("'%.*s' is not binary data",
                                            aLen, aValue));

    stdx::char_auto_ptr result (new char [len]);

    const char *src = aValue;
    char *dst = result.get();

    for (size_t i = 0; i < len; ++ i, ++ dst)
    {
        *dst = sFromHex (*src ++) << 4;
        *dst |= sFromHex (*src ++);
    }

    if (aLen != NULL)
        *aLen = len;

    return result;
}

//////////////////////////////////////////////////////////////////////////////
// type -> string conversions
//////////////////////////////////////////////////////////////////////////////

stdx::char_auto_ptr ToStringInteger (uint64_t aValue, unsigned int aBase,
                                     bool aSigned, int aBits)
{
    unsigned int flags = RTSTR_F_SPECIAL;
    if (aSigned)
        flags |= RTSTR_F_VALSIGNED;

    /* maximum is binary representation + terminator */
    size_t len = aBits + 1;

    switch (aBits)
    {
        case 8:
            flags |= RTSTR_F_8BIT;
            break;
        case 16:
            flags |= RTSTR_F_16BIT;
            break;
        case 32:
            flags |= RTSTR_F_32BIT;
            break;
        case 64:
            flags |= RTSTR_F_64BIT;
            break;
        default:
            throw xml::ENotImplemented (RT_SRC_POS);
    }

    stdx::char_auto_ptr result (new char [len]);
    if (aBase == 0)
        aBase = 10;
    int vrc = RTStrFormatNumber (result.get(), aValue, aBase, 0, 0, flags);
    if (RT_SUCCESS (vrc))
        return result;

    throw xml::EIPRTFailure (vrc);
}

template<> stdx::char_auto_ptr ToString <bool> (const bool &aValue,
                                                unsigned int aExtra /* = 0 */)
{
    /* Convert to the canonical form according to XML Schema */
    stdx::char_auto_ptr result (duplicate_chars (aValue ? "true" : "false"));
    return result;
}

template<> stdx::char_auto_ptr ToString <RTTIMESPEC> (const RTTIMESPEC &aValue,
                                                      unsigned int aExtra /* = 0 */)
{
    RTTIME time;
    if (!RTTimeExplode (&time, &aValue))
        throw ENoConversion(com::Utf8StrFmt("timespec %lld ms is invalid",
                                            RTTimeSpecGetMilli (&aValue)));

    /* Store ISO date (xsd:dateTime). The format is:
     * '-'? yyyy '-' mm '-' dd 'T' hh ':' mm ':' ss ('.' s+)? (zzzzzz)?
     * where zzzzzz is: (('+' | '-') hh ':' mm) | 'Z' */
    char buf [256];
    RTStrPrintf (buf, sizeof (buf),
                "%04ld-%02hd-%02hdT%02hd:%02hd:%02hdZ",
                time.i32Year, (uint16_t) time.u8Month, (uint16_t) time.u8MonthDay,
                (uint16_t) time.u8Hour, (uint16_t) time.u8Minute, (uint16_t) time.u8Second);

    stdx::char_auto_ptr result (duplicate_chars (buf));
    return result;
}

stdx::char_auto_ptr ToString (const char *aData, size_t aLen)
{
    /* each byte will produce two hex digits and there will be a null
     * terminator */
    stdx::char_auto_ptr result (new char [aLen * 2 + 1]);

    const char *src = aData;
    char *dst =  result.get();

    for (size_t i = 0; i < aLen; ++ i, ++ src)
    {
        *dst++ = sToHex ((*src) >> 4);
        *dst++ = sToHex ((*src) & 0xF);
    }

    *dst = '\0';

    return result;
}

//////////////////////////////////////////////////////////////////////////////
// XmlKeyBackend Class
//////////////////////////////////////////////////////////////////////////////

class XmlKeyBackend : public Key::Backend
{
public:

    XmlKeyBackend (xmlNodePtr aNode);
    ~XmlKeyBackend();

    const char *name() const;
    void setName (const char *aName);
    const char *value (const char *aName) const;
    void setValue (const char *aName, const char *aValue);

    Key::List keys (const char *aName = NULL) const;
    Key findKey (const char *aName) const;

    Key appendKey (const char *aName);
    void zap();

    void *position() const { return mNode; }

private:

    xmlNodePtr mNode;

    xmlChar *mNodeText;

    DECLARE_CLS_COPY_CTOR_ASSIGN_NOOP (XmlKeyBackend);

    friend class XmlTreeBackend;
};

XmlKeyBackend::XmlKeyBackend (xmlNodePtr aNode)
    : mNode (aNode), mNodeText (NULL)
{
    AssertReturnVoid (mNode);
    AssertReturnVoid (mNode->type == XML_ELEMENT_NODE);
}

XmlKeyBackend::~XmlKeyBackend()
{
    xmlFree (mNodeText);
}

const char *XmlKeyBackend::name() const
{
    return mNode ? (char *) mNode->name : NULL;
}

void XmlKeyBackend::setName (const char *aName)
{
    throw xml::ENotImplemented (RT_SRC_POS);
}

const char *XmlKeyBackend::value (const char *aName) const
{
    if (!mNode)
        return NULL;

    if (aName == NULL)
    {
        /* @todo xmlNodeListGetString (,,1) returns NULL for things like
         * <Foo></Foo> and may want to return "" in this case to distinguish
         * from <Foo/> (where NULL is pretty much expected). */
        if (!mNodeText)
            unconst (mNodeText) =
                xmlNodeListGetString (mNode->doc, mNode->children, 0);
        return (char *) mNodeText;
    }

    xmlAttrPtr attr = xmlHasProp (mNode, (const xmlChar *) aName);
    if (!attr)
        return NULL;

    if (attr->type == XML_ATTRIBUTE_NODE)
    {
        /* @todo for now, we only understand the most common case: only 1 text
         * node comprises the attribute's contents. Otherwise we'd need to
         * return a newly allocated string buffer to the caller that
         * concatenates all text nodes and obey him to free it or provide our
         * own internal map of attribute=value pairs and return const pointers
         * to values from this map. */
        if (attr->children != NULL &&
            attr->children->next == NULL &&
            (attr->children->type == XML_TEXT_NODE ||
             attr->children->type == XML_CDATA_SECTION_NODE))
            return (char *) attr->children->content;
    }
    else if (attr->type == XML_ATTRIBUTE_DECL)
    {
        return (char *) ((xmlAttributePtr) attr)->defaultValue;
    }

    return NULL;
}

void XmlKeyBackend::setValue (const char *aName, const char *aValue)
{
    if (!mNode)
        return;

    if (aName == NULL)
    {
        xmlChar *value = (xmlChar *) aValue;
        if (value != NULL)
        {
            value = xmlEncodeSpecialChars (mNode->doc, value);
            if (value == NULL)
                throw xml::ENoMemory();
        }

        xmlNodeSetContent (mNode, value);

        if (value != (xmlChar *) aValue)
            xmlFree (value);

        /* outdate the node text holder */
        if (mNodeText != NULL)
        {
            xmlFree (mNodeText);
            mNodeText = NULL;
        }

        return;
    }

    if (aValue == NULL)
    {
        /* remove the attribute if it exists */
        xmlAttrPtr attr = xmlHasProp (mNode, (const xmlChar *) aName);
        if (attr != NULL)
        {
            int rc = xmlRemoveProp (attr);
            if (rc != 0)
                throw xml::EInvalidArg (RT_SRC_POS);
        }
        return;
    }

    xmlAttrPtr attr = xmlSetProp (mNode, (const xmlChar *) aName,
                                  (const xmlChar *) aValue);
    if (attr == NULL)
        throw xml::ENoMemory();
}

Key::List XmlKeyBackend::keys (const char *aName /* = NULL */) const
{
    Key::List list;

    if (!mNode)
        return list;

    for (xmlNodePtr node = mNode->children; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (aName == NULL ||
                strcmp (aName, (char *) node->name) == 0)
                list.push_back (Key (new XmlKeyBackend (node)));
        }
    }

    return list;
}

Key XmlKeyBackend::findKey (const char *aName) const
{
    Key key;

    if (!mNode)
        return key;

    for (xmlNodePtr node = mNode->children; node; node = node->next)
    {
        if (node->type == XML_ELEMENT_NODE)
        {
            if (aName == NULL ||
                strcmp (aName, (char *) node->name) == 0)
            {
                key = Key (new XmlKeyBackend (node));
                break;
            }
        }
    }

    return key;
}

Key XmlKeyBackend::appendKey (const char *aName)
{
    if (!mNode)
        return Key();

    xmlNodePtr node = xmlNewChild (mNode, NULL, (const xmlChar *) aName, NULL);
    if (node == NULL)
        throw xml::ENoMemory();

    return Key (new XmlKeyBackend (node));
}

void XmlKeyBackend::zap()
{
    if (!mNode)
        return;

    xmlUnlinkNode (mNode);
    xmlFreeNode (mNode);
    mNode = NULL;
}

//////////////////////////////////////////////////////////////////////////////
// XmlTreeBackend Class
//////////////////////////////////////////////////////////////////////////////

struct XmlTreeBackend::Data
{
    Data() : ctxt (NULL), doc (NULL)
           , inputResolver (NULL)
           , autoConverter (NULL), oldVersion (NULL) {}

    xmlParserCtxtPtr ctxt;
    xmlDocPtr doc;

    Key root;

    InputResolver *inputResolver;

    AutoConverter *autoConverter;
    char *oldVersion;

    std::auto_ptr <stdx::exception_trap_base> trappedErr;

    /**
     * This is to avoid throwing exceptions while in libxml2 code and
     * redirect them to our level instead. Also used to perform clean up
     * by deleting the I/O stream instance and self when requested.
     */
    struct IOCtxt
    {
        IOCtxt (xml::Stream *aStream, std::auto_ptr <stdx::exception_trap_base> &aErr)
            : stream (aStream), deleteStreamOnClose (false)
            , err (aErr) {}

        template <typename T>
        void setErr (const T& aErr) { err.reset (new stdx::exception_trap <T> (aErr)); }

        void resetErr() { err.reset(); }

        xml::Stream *stream;
        bool deleteStreamOnClose;

        std::auto_ptr <stdx::exception_trap_base> &err;
    };

    struct InputCtxt : public IOCtxt
    {
        InputCtxt (xml::Input *aInput, std::auto_ptr <stdx::exception_trap_base> &aErr)
            : IOCtxt (aInput, aErr), input (aInput) {}

        xml::Input *input;
    };

    struct OutputCtxt : public IOCtxt
    {
        OutputCtxt (xml::Output *aOutput, std::auto_ptr <stdx::exception_trap_base> &aErr)
            : IOCtxt (aOutput, aErr), output (aOutput) {}

        xml::Output *output;
    };
};

XmlTreeBackend::XmlTreeBackend()
    : m (new Data())
{
    /* create a parser context */
    m->ctxt = xmlNewParserCtxt();
    if (m->ctxt == NULL)
        throw xml::ENoMemory();
}

XmlTreeBackend::~XmlTreeBackend()
{
    reset();

    xmlFreeParserCtxt (m->ctxt);
    m->ctxt = NULL;
}

void XmlTreeBackend::setInputResolver (InputResolver &aResolver)
{
    m->inputResolver = &aResolver;
}

void XmlTreeBackend::resetInputResolver()
{
    m->inputResolver = NULL;
}

void XmlTreeBackend::setAutoConverter (AutoConverter &aConverter)
{
    m->autoConverter = &aConverter;
}

void XmlTreeBackend::resetAutoConverter()
{
    m->autoConverter = NULL;
}

const char *XmlTreeBackend::oldVersion() const
{
    return m->oldVersion;
}

extern "C" xmlGenericErrorFunc xsltGenericError;
extern "C" void *xsltGenericErrorContext;

void XmlTreeBackend::rawRead (xml::Input &aInput, const char *aSchema /* = NULL */,
                              int aFlags /* = 0 */)
{
    /* Reset error variables used to memorize exceptions while inside the
     * libxml2 code. */
    m->trappedErr.reset();

    /* We use the global lock for the whole duration of this method to serialize
     * access to thread-unsafe xmlGetExternalEntityLoader() and some other
     * calls. It means that only one thread is able to parse an XML stream at a
     * time but another choice would be to patch libxml2/libxslt which is
     * unwanted now for several reasons. Search for "thread-safe" to find all
     * unsafe cases. */
    xml::GlobalLock global;
    global.setExternalEntityLoader(ExternalEntityLoader);

    sThat = this;
    xmlDocPtr doc = NULL;

    try
    {
        /* Note: when parsing we use XML_PARSE_NOBLANKS to instruct libxml2 to
         * remove text nodes that contain only blanks. This is important because
         * otherwise xmlSaveDoc() won't be able to do proper indentation on
         * output. */
        /* parse the stream */
        /* NOTE: new InputCtxt instance will be deleted when the stream is closed by
         * the libxml2 API (e.g. when calling xmlFreeParserCtxt()) */
        doc = xmlCtxtReadIO (m->ctxt,
                             ReadCallback, CloseCallback,
                             new Data::InputCtxt (&aInput, m->trappedErr),
                             aInput.uri(), NULL,
                             XML_PARSE_NOBLANKS);
        if (doc == NULL)
        {
            /* look if there was a forwared exception from the lower level */
            if (m->trappedErr.get() != NULL)
                m->trappedErr->rethrow();

            throw xml::XmlError(xmlCtxtGetLastError (m->ctxt));
        }

        char *oldVersion = NULL;

        /* perform automatic document transformation if necessary */
        if (m->autoConverter != NULL &&
            m->autoConverter->
                needsConversion (Key (new XmlKeyBackend (xmlDocGetRootElement (doc))),
                                 &oldVersion))
        {
            xmlDocPtr xsltDoc = NULL;
            xsltStylesheetPtr xslt = NULL;
            char *errorStr = NULL;

            xmlGenericErrorFunc oldXsltGenericError = xsltGenericError;
            void *oldXsltGenericErrorContext = xsltGenericErrorContext;

            try
            {
                /* parse the XSLT template */
                {
                    xml::Input *xsltInput =
                        m->inputResolver->resolveEntity
                            (m->autoConverter->templateUri(), NULL);
                    /* NOTE: new InputCtxt instance will be deleted when the
                     * stream is closed by the libxml2 API */
                    xsltDoc = xmlCtxtReadIO (m->ctxt,
                                             ReadCallback, CloseCallback,
                                             new Data::InputCtxt (xsltInput, m->trappedErr),
                                             m->autoConverter->templateUri(),
                                             NULL, 0);
                    delete xsltInput;
                }

                if (xsltDoc == NULL)
                {
                    /* look if there was a forwared exception from the lower level */
                    if (m->trappedErr.get() != NULL)
                        m->trappedErr->rethrow();

                    throw xml::XmlError(xmlCtxtGetLastError (m->ctxt));
                }

                /* setup stylesheet compilation and transformation error
                 * reporting. Note that we could create a new transform context
                 * for doing xsltApplyStylesheetUser and use
                 * xsltSetTransformErrorFunc() on it to set a dedicated error
                 * handler but as long as we already do several non-thread-safe
                 * hacks, this is not really important. */

                xsltGenericError = ValidityErrorCallback;
                xsltGenericErrorContext = &errorStr;

                xslt = xsltParseStylesheetDoc (xsltDoc);
                if (xslt == NULL)
                {
                    if (errorStr != NULL)
                        throw xml::LogicError (errorStr);
                    /* errorStr is freed in catch(...) below */

                    throw xml::LogicError (RT_SRC_POS);
                }

                /* repeat transformations until autoConverter is satisfied */
                do
                {
                    xmlDocPtr newDoc = xsltApplyStylesheet (xslt, doc, NULL);
                    if (newDoc == NULL && errorStr == NULL)
                        throw xml::LogicError (RT_SRC_POS);

                    if (errorStr != NULL)
                    {
                        xmlFreeDoc (newDoc);
                        throw xml::RuntimeError(errorStr);
                        /* errorStr is freed in catch(...) below */
                    }

                    /* replace the old document on success */
                    xmlFreeDoc (doc);
                    doc = newDoc;
                }
                while (m->autoConverter->
                       needsConversion (Key (new XmlKeyBackend (xmlDocGetRootElement (doc))),
                                        NULL));

                RTStrFree (errorStr);

                /* NOTE: xsltFreeStylesheet() also fress the document
                 * passed to xsltParseStylesheetDoc(). */
                xsltFreeStylesheet (xslt);

                /* restore the previous generic error func */
                xsltGenericError = oldXsltGenericError;
                xsltGenericErrorContext = oldXsltGenericErrorContext;
            }
            catch (...)
            {
                RTStrFree (errorStr);

                /* NOTE: xsltFreeStylesheet() also fress the document
                 * passed to xsltParseStylesheetDoc(). */
                if (xslt != NULL)
                    xsltFreeStylesheet (xslt);
                else if (xsltDoc != NULL)
                    xmlFreeDoc (xsltDoc);

                /* restore the previous generic error func */
                xsltGenericError = oldXsltGenericError;
                xsltGenericErrorContext = oldXsltGenericErrorContext;

                RTStrFree (oldVersion);

                throw;
            }
        }

        /* validate the document */
        if (aSchema != NULL)
        {
            xmlSchemaParserCtxtPtr schemaCtxt = NULL;
            xmlSchemaPtr schema = NULL;
            xmlSchemaValidCtxtPtr validCtxt = NULL;
            char *errorStr = NULL;

            try
            {
                bool valid = false;

                schemaCtxt = xmlSchemaNewParserCtxt (aSchema);
                if (schemaCtxt == NULL)
                    throw xml::LogicError (RT_SRC_POS);

                /* set our error handlers */
                xmlSchemaSetParserErrors (schemaCtxt, ValidityErrorCallback,
                                          ValidityWarningCallback, &errorStr);
                xmlSchemaSetParserStructuredErrors (schemaCtxt,
                                                    StructuredErrorCallback,
                                                    &errorStr);
                /* load schema */
                schema = xmlSchemaParse (schemaCtxt);
                if (schema != NULL)
                {
                    validCtxt = xmlSchemaNewValidCtxt (schema);
                    if (validCtxt == NULL)
                        throw xml::LogicError (RT_SRC_POS);

                    /* instruct to create default attribute's values in the document */
                    if (aFlags & Read_AddDefaults)
                        xmlSchemaSetValidOptions (validCtxt, XML_SCHEMA_VAL_VC_I_CREATE);

                    /* set our error handlers */
                    xmlSchemaSetValidErrors (validCtxt, ValidityErrorCallback,
                                             ValidityWarningCallback, &errorStr);

                    /* finally, validate */
                    valid = xmlSchemaValidateDoc (validCtxt, doc) == 0;
                }

                if (!valid)
                {
                    /* look if there was a forwared exception from the lower level */
                    if (m->trappedErr.get() != NULL)
                        m->trappedErr->rethrow();

                    if (errorStr == NULL)
                        throw xml::LogicError (RT_SRC_POS);

                    throw xml::RuntimeError(errorStr);
                    /* errorStr is freed in catch(...) below */
                }

                RTStrFree (errorStr);

                xmlSchemaFreeValidCtxt (validCtxt);
                xmlSchemaFree (schema);
                xmlSchemaFreeParserCtxt (schemaCtxt);
            }
            catch (...)
            {
                RTStrFree (errorStr);

                if (validCtxt)
                    xmlSchemaFreeValidCtxt (validCtxt);
                if (schema)
                    xmlSchemaFree (schema);
                if (schemaCtxt)
                    xmlSchemaFreeParserCtxt (schemaCtxt);

                RTStrFree (oldVersion);

                throw;
            }
        }

        /* reset the previous tree on success */
        reset();

        m->doc = doc;
        /* assign the root key */
        m->root = Key (new XmlKeyBackend (xmlDocGetRootElement (m->doc)));

        /* memorize the old version string also used as a flag that
         * the conversion has been performed (transfers ownership) */
        m->oldVersion = oldVersion;

        sThat = NULL;
    }
    catch (...)
    {
        if (doc != NULL)
            xmlFreeDoc (doc);

        sThat = NULL;

        throw;
    }
}

void XmlTreeBackend::rawWrite (xml::Output &aOutput)
{
    /* reset error variables used to memorize exceptions while inside the
     * libxml2 code */
    m->trappedErr.reset();

    /* set up an input stream for parsing the document. This will be deleted
     * when the stream is closed by the libxml2 API (e.g. when calling
     * xmlFreeParserCtxt()). */
    Data::OutputCtxt *outputCtxt =
        new Data::OutputCtxt (&aOutput, m->trappedErr);

    /* serialize to the stream */

    xmlIndentTreeOutput = 1;
    xmlTreeIndentString = "  ";
    xmlSaveNoEmptyTags = 0;

    xmlSaveCtxtPtr saveCtxt = xmlSaveToIO (WriteCallback, CloseCallback,
                                           outputCtxt, NULL,
                                           XML_SAVE_FORMAT);
    if (saveCtxt == NULL)
        throw xml::LogicError (RT_SRC_POS);

    long rc = xmlSaveDoc (saveCtxt, m->doc);
    if (rc == -1)
    {
        /* look if there was a forwared exception from the lower level */
        if (m->trappedErr.get() != NULL)
            m->trappedErr->rethrow();

        /* there must be an exception from the Output implementation,
         * otherwise the save operation must always succeed. */
        throw xml::LogicError (RT_SRC_POS);
    }

    xmlSaveClose (saveCtxt);
}

void XmlTreeBackend::reset()
{
    RTStrFree (m->oldVersion);
    m->oldVersion = NULL;

    if (m->doc)
    {
        /* reset the root key's node */
        GetKeyBackend (m->root)->mNode = NULL;
        /* free the document*/
        xmlFreeDoc (m->doc);
        m->doc = NULL;
    }
}

Key &XmlTreeBackend::rootKey() const
{
    return m->root;
}

/* static */
int XmlTreeBackend::ReadCallback (void *aCtxt, char *aBuf, int aLen)
{
    AssertReturn (aCtxt != NULL, 0);

    Data::InputCtxt *ctxt = static_cast <Data::InputCtxt *> (aCtxt);

    /* To prevent throwing exceptions while inside libxml2 code, we catch
     * them and forward to our level using a couple of variables. */
    try
    {
        return ctxt->input->read (aBuf, aLen);
    }
    catch (const xml::EIPRTFailure &err) { ctxt->setErr (err); }
    catch (const xml::Error &err) { ctxt->setErr (err); }
    catch (const std::exception &err) { ctxt->setErr (err); }
    catch (...) { ctxt->setErr (xml::LogicError (RT_SRC_POS)); }

    return -1 /* failure */;
}

/* static */
int XmlTreeBackend::WriteCallback (void *aCtxt, const char *aBuf, int aLen)
{
    AssertReturn (aCtxt != NULL, 0);

    Data::OutputCtxt *ctxt = static_cast <Data::OutputCtxt *> (aCtxt);

    /* To prevent throwing exceptions while inside libxml2 code, we catch
     * them and forward to our level using a couple of variables. */
    try
    {
        return ctxt->output->write (aBuf, aLen);
    }
    catch (const xml::EIPRTFailure &err) { ctxt->setErr (err); }
    catch (const xml::Error &err) { ctxt->setErr (err); }
    catch (const std::exception &err) { ctxt->setErr (err); }
    catch (...) { ctxt->setErr (xml::LogicError (RT_SRC_POS)); }

    return -1 /* failure */;
}

/* static */
int XmlTreeBackend::CloseCallback (void *aCtxt)
{
    AssertReturn (aCtxt != NULL, 0);

    Data::IOCtxt *ctxt = static_cast <Data::IOCtxt *> (aCtxt);

    /* To prevent throwing exceptions while inside libxml2 code, we catch
     * them and forward to our level using a couple of variables. */
    try
    {
        /// @todo there is no explicit close semantics in Stream yet
#if 0
        ctxt->stream->close();
#endif

        /* perform cleanup when necessary */
        if (ctxt->deleteStreamOnClose)
            delete ctxt->stream;

        delete ctxt;

        return 0 /* success */;
    }
    catch (const xml::EIPRTFailure &err) { ctxt->setErr (err); }
    catch (const xml::Error &err) { ctxt->setErr (err); }
    catch (const std::exception &err) { ctxt->setErr (err); }
    catch (...) { ctxt->setErr (xml::LogicError (RT_SRC_POS)); }

    return -1 /* failure */;
}

/* static */
void XmlTreeBackend::ValidityErrorCallback (void *aCtxt, const char *aMsg, ...)
{
    AssertReturnVoid (aCtxt != NULL);
    AssertReturnVoid (aMsg != NULL);

    char * &str = *(char * *) aCtxt;

    char *newMsg = NULL;
    {
        va_list args;
        va_start (args, aMsg);
        RTStrAPrintfV (&newMsg, aMsg, args);
        va_end (args);
    }

    AssertReturnVoid (newMsg != NULL);

    /* strip spaces, trailing EOLs and dot-like char */
    size_t newMsgLen = strlen (newMsg);
    while (newMsgLen && strchr (" \n.?!", newMsg [newMsgLen - 1]))
        -- newMsgLen;

    /* anything left? */
    if (newMsgLen > 0)
    {
        if (str == NULL)
        {
            str = newMsg;
            newMsg [newMsgLen] = '\0';
        }
        else
        {
            /* append to the existing string */
            size_t strLen = strlen (str);
            char *newStr = (char *) RTMemRealloc (str, strLen + 2 + newMsgLen + 1);
            AssertReturnVoid (newStr != NULL);

            memcpy (newStr + strLen, ".\n", 2);
            memcpy (newStr + strLen + 2, newMsg, newMsgLen);
            newStr [strLen + 2 + newMsgLen] = '\0';
            str = newStr;
            RTStrFree (newMsg);
        }
    }
}

/* static */
void XmlTreeBackend::ValidityWarningCallback (void *aCtxt, const char *aMsg, ...)
{
    NOREF (aCtxt);
    NOREF (aMsg);
}

/* static */
void XmlTreeBackend::StructuredErrorCallback (void *aCtxt, xmlErrorPtr aErr)
{
    AssertReturnVoid (aCtxt != NULL);
    AssertReturnVoid (aErr != NULL);

    char * &str = *(char * *) aCtxt;

    char *newMsg = xml::XmlError::Format (aErr);
    AssertReturnVoid (newMsg != NULL);

    if (str == NULL)
        str = newMsg;
    else
    {
        /* append to the existing string */
        size_t newMsgLen = strlen (newMsg);
        size_t strLen = strlen (str);
        char *newStr = (char *) RTMemRealloc (str, strLen + newMsgLen + 2);
        AssertReturnVoid (newStr != NULL);

        memcpy (newStr + strLen, ".\n", 2);
        memcpy (newStr + strLen + 2, newMsg, newMsgLen);
        str = newStr;
        RTStrFree (newMsg);
    }
}

/* static */
XmlTreeBackend *XmlTreeBackend::sThat = NULL;

/* static */
xmlParserInputPtr XmlTreeBackend::ExternalEntityLoader (const char *aURI,
                                                        const char *aID,
                                                        xmlParserCtxtPtr aCtxt)
{
    AssertReturn (sThat != NULL, NULL);

    if (sThat->m->inputResolver == NULL)
        return xml::GlobalLock::callDefaultLoader(aURI, aID, aCtxt);

    /* To prevent throwing exceptions while inside libxml2 code, we catch
     * them and forward to our level using a couple of variables. */
    try
    {
        xml::Input *input = sThat->m->inputResolver->resolveEntity (aURI, aID);
        if (input == NULL)
            return NULL;

        Data::InputCtxt *ctxt = new Data::InputCtxt (input, sThat->m->trappedErr);
        ctxt->deleteStreamOnClose = true;

        /* create an input buffer with custom hooks */
        xmlParserInputBufferPtr bufPtr =
            xmlParserInputBufferCreateIO (ReadCallback, CloseCallback,
                                          ctxt, XML_CHAR_ENCODING_NONE);
        if (bufPtr)
        {
            /* create an input stream */
            xmlParserInputPtr inputPtr =
                xmlNewIOInputStream (aCtxt, bufPtr, XML_CHAR_ENCODING_NONE);

            if (inputPtr != NULL)
            {
                /* pass over the URI to the stream struct (it's NULL by
                 * default) */
                inputPtr->filename =
                    (char *) xmlCanonicPath ((const xmlChar *) input->uri());
                return inputPtr;
            }
        }

        /* either of libxml calls failed */

        if (bufPtr)
            xmlFreeParserInputBuffer (bufPtr);

        delete input;
        delete ctxt;

        throw xml::ENoMemory();
    }
    catch (const xml::EIPRTFailure &err) { sThat->m->trappedErr.reset (stdx::new_exception_trap (err)); }
    catch (const xml::Error &err) { sThat->m->trappedErr.reset (stdx::new_exception_trap (err)); }
    catch (const std::exception &err) { sThat->m->trappedErr.reset (stdx::new_exception_trap (err)); }
    catch (...) { sThat->m->trappedErr.reset (stdx::new_exception_trap (xml::LogicError (RT_SRC_POS))); }

    return NULL;
}


} /* namespace settings */
