
/*
 * Sample client for the VirtualBox webservice, written in Java.
 * Best consumed in conjunction with the explanations in the VirtualBox
 * User Manual, which describe in detail how to get this code running.
 *
 * Copyright (C) 2008 Sun Microsystems, Inc.
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

import org.virtualbox.www.Service.VboxService;
import org.virtualbox.www.Service.VboxServiceLocator;
import org.virtualbox.www.VboxPortType;

public class clienttest
{
    private VboxService  _service;
    private VboxPortType _port;
    private String       _oVbox;

    public clienttest()
    {
        try
        {
            // instantiate the webservice in instance data; the classes
            // VboxServiceLocator and VboxPortType have been created
            // by the WSDL2Java helper that you should have run prior
            // to compiling this example, as described in the User Manual.
            _service = new VboxServiceLocator();
            _port = _service.getvboxServicePort();

            // From now on, we can call any method in the webservice by
            // prefixing it with "port."

            // First step is always to log on to the webservice. This
            // returns a managed object reference to the webservice's
            // global instance of IVirtualBox, which in turn contains
            // the most important methods provided by the Main API.
            _oVbox = _port.IWebsessionManager_logon("", "");

            // Call IVirtualBox::getVersion and print out the result
            String version = _port.IVirtualBox_getVersion(_oVbox);
            System.out.println("Initialized connection to VirtualBox version " + version);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void showVMs()
    {
        try
        {
            // Call IVirtualBox::getMachines, which yields an array
            // of managed object references to all machines which have
            // been registered:
            String[] aMachines = _port.IVirtualBox_getMachines2(_oVbox);
            // Walk through this array and, for each machine, call
            // IMachine::getName (accessor method to the "name" attribute)
            for (int i = 0; i < aMachines.length; i++)
            {
                String oMachine = aMachines[i];
                String machinename = _port.IMachine_getName(oMachine);
                System.out.println("Machine " + i + ": " + oMachine + " - " + machinename);

                // release managed object reference
                _port.IManagedObjectRef_release(oMachine);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void listHostInfo()
    {
        try
        {
            String oHost = _port.IVirtualBox_getHost(_oVbox);

            org.apache.axis.types.UnsignedInt uProcCount = _port.IHost_getProcessorCount(oHost);
            System.out.println("Processor count: " + uProcCount);

            String oCollector = _port.IVirtualBox_getPerformanceCollector(_oVbox);

            String aobj[] = {oHost};
            String astrMetrics[] = {"*"};
            String aMetrics[] = {};
            aMetrics = _port.IPerformanceCollector_getMetrics(oCollector,
                                                   astrMetrics,
                                                   aobj);

//             String astrMetricNames[] = { "*" };
//             String aObjects[];
//             String aRetNames[];
//             int aRetIndices[];
//             int aRetLengths[];
//             int aRetData[];
//             int rc = _port.ICollector_queryMetricsData(oCollector,
//                                                        aObjects,
//                                                        aRetNames,
//                                                        aRetObjects,
//                                                        aRetIndices,
//                                                        aRetLengths,
//                                                        aRetData);
//
/*
    Bstr metricNames[] = { L"*" };
    com::SafeArray<BSTR> metrics (1);
    metricNames[0].cloneTo (&metrics [0]);
    com::SafeArray<BSTR>          retNames;
    com::SafeIfaceArray<IUnknown> retObjects;
    com::SafeArray<ULONG>         retIndices;
    com::SafeArray<ULONG>         retLengths;
    com::SafeArray<LONG>          retData;
    CHECK_ERROR (collector, QueryMetricsData(ComSafeArrayAsInParam(metrics),
                                             ComSafeArrayInArg(objects),
                                             ComSafeArrayAsOutParam(retNames),
                                             ComSafeArrayAsOutParam(retObjects),
                                             ComSafeArrayAsOutParam(retIndices),
                                             ComSafeArrayAsOutParam(retLengths),
                                             ComSafeArrayAsOutParam(retData)) );
*/

        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public void startVM(String strVM)
    {
        String oSession = "";
        Boolean fSessionOpen = false;

        try
        {
            // this is pretty much what VBoxManage does to start a VM
            String oMachine = "";
            Boolean fOK = false;

            oSession = _port.IWebsessionManager_getSessionObject(_oVbox);

            // first assume we were given a UUID
            try
            {
                oMachine = _port.IVirtualBox_getMachine(_oVbox, strVM);
                fOK = true;
            }
            catch (Exception e)
            {
            }

            if (!fOK)
            {
                try
                {
                    // or try by name
                    oMachine = _port.IVirtualBox_findMachine(_oVbox, strVM);
                    fOK = true;
                }
                catch (Exception e)
                {
                }
            }

            if (!fOK)
            {
                System.out.println("Error: can't find VM \"" + strVM + "\"");
            }
            else
            {
                String uuid = _port.IMachine_getId(oMachine);
                String sessionType = "gui";
                String env = "DISPLAY=:0.0";
                String oProgress = _port.IVirtualBox_openRemoteSession(_oVbox, oSession, uuid, sessionType, env);
                fSessionOpen = true;

                System.out.println("Session for VM " + uuid + " is opening...");
                _port.IProgress_waitForCompletion(oProgress, 10000);

                int rc = _port.IProgress_getResultCode(oProgress).intValue();
                if (rc != 0)
                {
                    System.out.println("Session failed!");
                }
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }

        if (fSessionOpen)
        {
            try
            {
                _port.ISession_close(oSession);
            }
            catch (Exception e)
            {
            }
        }
    }

    public void cleanup()
    {
        try
        {
            if (_oVbox.length() > 0)
            {
                // log off
                _port.IWebsessionManager_logoff(_oVbox);
                _oVbox = null;
                System.out.println("Logged off.");
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    public static void printArgs()
    {
        System.out.println(  "Usage: java clienttest <mode> ..." +
                           "\nwith <mode> being:" +
                           "\n   show vms            list installed virtual machines" +
                           "\n   list hostinfo       list host info" +
                           "\n   startvm <vmname|uuid> start the given virtual machine");
    }

    public static void main(String[] args)
    {
        if (args.length < 1)
        {
            System.out.println("Error: Must specify at least one argument.");
            printArgs();
        }
        else
        {
            clienttest c = new clienttest();
            if (args[0].equals("show"))
            {
                if (args.length == 2)
                {
                    if (args[1].equals("vms"))
                        c.showVMs();
                    else
                        System.out.println("Error: Unknown argument to \"show\": \"" + args[1] + "\".");
                }
                else
                    System.out.println("Error: Missing argument to \"show\" command");
            }
            else if (args[0].equals("list"))
            {
                if (args.length == 2)
                {
                    if (args[1].equals("hostinfo"))
                        c.listHostInfo();
                    else
                        System.out.println("Error: Unknown argument to \"show\": \"" + args[1] + "\".");
                }
                else
                    System.out.println("Error: Missing argument to \"show\" command");
            }
            else if (args[0].equals("startvm"))
            {
                if (args.length == 2)
                {
                    c.startVM(args[1]);
                }
                else
                    System.out.println("Error: Missing argument to \"startvm\" command");
            }
            else
                System.out.println("Error: Unknown command: \"" + args[0] + "\".");

            c.cleanup();
        }
    }
}