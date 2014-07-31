/*
 * Copyright (c) 2013      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * This file should be included by any file that needs full
 * version information for the OSHMEM project
 */

#ifndef OSHMEM_VERSIONS_H
#define OSHMEM_VERSIONS_H

#define OSHMEM_MAJOR_VERSION 1
#define OSHMEM_MINOR_VERSION 9
#define OSHMEM_RELEASE_VERSION 0
#define OSHMEM_GREEK_VERSION "a1"
#define OSHMEM_WANT_REPO_REV 1
#define OSHMEM_REPO_REV "hgc7fa38d11345"
#ifdef OSHMEM_VERSION
/* If we included version.h, we want the real version, not the
   stripped (no-r number) version */
#undef OSHMEM_VERSION
#endif
#define OSHMEM_VERSION "1.9a1hgc7fa38d11345"

#endif
