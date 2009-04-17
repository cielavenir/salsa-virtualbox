 * The preferred slash character.
 * The preferred slash character as a string, handy for concatenations
 * @remark  This is sufficient for the drive letter concept on PC.
 * Strips the filename from a path. Truncates the given string in-place by overwriting the
 * last path separator character with a null byte in a platform-neutral way.
 * @param   pszPath     Path from which filename should be extracted, will be truncated.
 *                      If the string contains no path separator, it will be changed to a "." string.
 * case-insensitive compares on unix systems when a path goes into a case-insensitive
 * you'll won't get case-sensitive compares on a case-sensitive file system.
 *          consistent with the other APIs. RTPathStartsWith(pszSomePath, "/") will
 * @param   pModificationTime   Pointer to the new modification time.
 * @param   pModificationTime   Where to store the modification time. NULL is ok.