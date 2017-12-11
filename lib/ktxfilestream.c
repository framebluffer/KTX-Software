/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file
 * @~English
 *
 * @brief Implementation of ktxStream for FILE.
 *
 * @author Maksim Kolesin, Under Development
 * @author Georg Kolling, Imagination Technology
 * @author Mark Callow, HI Corporation
 */

/*
Copyright (c) 2010 The Khronos Group Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and/or associated documentation files (the
"Materials"), to deal in the Materials without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Materials, and to
permit persons to whom the Materials are furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
unaltered in all copies or substantial portions of the Materials.
Any additions, deletions, or changes to the original source files
must be clearly indicated in accompanying documentation.

If only executable code is distributed, then the accompanying
documentation must state that "this software is based in part on the
work of the Khronos Group".

THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "ktx.h"
#include "ktxint.h"
#include "ktxfilestream.h"

/**
 * @internal
 * @~English
 * @brief Read bytes from a ktxFileStream.
 *
 * @param [in]  str     pointer to the ktxStream from which to read.
 * @param [out] dst     pointer to a block of memory with a size
 *                      of at least @p size bytes, converted to a void*.
 * @param [in]  size    total size of bytes to be read.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p dst is @c NULL or @p src is @c NULL.
 * @exception KTX_UNEXPECTED_END_OF_FILE the file does not contain the expected
 *                                       amount of data.
 */
static
KTX_error_code ktxFileStream_read(ktxStream* str, void* dst, const GLsizei size)
{
	if (!str || !dst)
		return KTX_INVALID_VALUE;

    assert(str->type == eStreamTypeFile);
    
	if (fread(dst, size, 1, str->data.file) != 1)
		return KTX_UNEXPECTED_END_OF_FILE;

	return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Skip bytes in a ktxFileStream.
 *
 * @param [in] str           pointer to a ktxStream object.
 * @param [in] count         number of bytes to be skipped.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p str is @c NULL or @p count is less than zero.
 * @exception KTX_UNEXPECTED_END_OF_FILE the file does not contain the expected
 *                                       amount of data.
 */
static
KTX_error_code ktxFileStream_skip(ktxStream* str, const GLsizei count)
{
	if (!str || (count < 0))
		return KTX_INVALID_VALUE;

    assert(str->type == eStreamTypeFile);
    
	if (fseek(str->data.file, count, SEEK_CUR) != 0)
		return KTX_UNEXPECTED_END_OF_FILE;

	return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Write bytes to a ktxFileStream.
 *
 * @param [in] str      pointer to the ktxStream that is the destination of the
 *                      write.
 * @param [in] src      pointer to the array of elements to be written,
 *                      converted to a const void*.
 * @param [in] size     size in bytes of each element to be written.
 * @param [in] count    number of elements, each one with a @p size of size
 *                      bytes.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p str is @c NULL or @p src is @c NULL.
 * @exception KTX_FILE_WRITE_ERROR a system error occurred while writing the
 *                                 file.
 */
static
KTX_error_code ktxFileStream_write(ktxStream* str, const void *src,
                                   const GLsizei size, const GLsizei count)
{
	if (!str || !src)
		return KTX_INVALID_VALUE;

    assert(str->type == eStreamTypeFile);
    
	if (fwrite(src, size, count, str->data.file) != count)
		return KTX_FILE_WRITE_ERROR;

	return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Get the current read/write position in a ktxFileStream.
 *
 * @param [in] str      pointer to the ktxStream to query.
 * @param [in,out] off  pointer to variable to receive the offset value.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p str or @p pos is @c NULL.
 */
static
KTX_error_code ktxFileStream_getpos(ktxStream* str, off_t* pos)
{
    if (!str || !pos)
        return KTX_INVALID_VALUE;
    
    assert(str->type == eStreamTypeFile);
    
    *pos = ftello(str->data.file);
    
    return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Set the current read/write position in a ktxFileStream.
 *
 * Offset of 0 is the start of the file.
 *
 * @param [in] str    pointer to the ktxStream whose r/w position is to be set.
 * @param [in] off    pointer to the offset value to set.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p str is @c NULL.
 * @exception KTX_INVALID_OPERATION @p pos is > the size of the file or an
 *                                  fseek error occurred.
 */
static
KTX_error_code ktxFileStream_setpos(ktxStream* str, off_t pos)
{
    size_t fileSize;
    
    if (!str)
        return KTX_INVALID_VALUE;

    assert(str->type == eStreamTypeFile);
    
    str->getsize(str, &fileSize);
    if (pos > fileSize)
        return KTX_INVALID_OPERATION;

    if (fseeko(str->data.file, pos, SEEK_SET) < 0)
        /* FIXME. Return a more suitable error. */
        return KTX_INVALID_OPERATION;
    
    return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Get the size of a ktxFileStream in bytes.
 *
 * @param [in] str       pointer to the ktxStream whose size is to be queried.
 * @param [in,out] size  pointer to a variable in which size will be written.
 *
 * @return      KTX_SUCCESS on success, other KTX_* enum values on error.
 *
 * @exception KTX_INVALID_VALUE @p str or @p size is @c NULL.
 * @exception KTX_FILE_WRITE_ERROR a system error occurred while getting the
 *                                 size.
 */
static
KTX_error_code ktxFileStream_getsize(ktxStream* str, size_t* size)
{
    struct stat statbuf;

    if (!str || !size)
        return KTX_INVALID_VALUE;
    
    assert(str->type == eStreamTypeFile);
 
    if (fstat(fileno(str->data.file), &statbuf) < 0)
        /* FIXME. Return a more suitable errors. */
        return KTX_FILE_WRITE_ERROR;
    *size = statbuf.st_size;
    
    return KTX_SUCCESS;
}

/**
 * @internal
 * @~English
 * @brief Initialize a ktxFileStream.
 *
 * @param [in] str      pointer to the ktxStream to initialize.
 * @param [in] file     pointer to the underlying FILE object.
 *
 * @return      KTX_SUCCESS on success, KTX_INVALID_VALUE on error.
 *
 * @exception KTX_INVALID_VALUE @p stream is @c NULL or @p file is @c NULL.
 */
KTX_error_code ktxFileStream_construct(ktxStream* str, FILE* file)
{
	if (!str || !file)
		return KTX_INVALID_VALUE;

	str->data.file = file;
    str->type = eStreamTypeFile;
	str->read = ktxFileStream_read;
	str->skip = ktxFileStream_skip;
	str->write = ktxFileStream_write;
    str->getpos = ktxFileStream_getpos;
    str->setpos = ktxFileStream_setpos;
    str->getsize = ktxFileStream_getsize;

	return KTX_SUCCESS;
}
