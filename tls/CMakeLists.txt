
project (tls LANGUAGES C)

find_package(OpenSSL REQUIRED)

include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)
include(CheckIncludeFiles)
include(CheckTypeSize)
    
set(
	TLS_SRC
	tls.c
	tls_bio_cb.c
	tls_client.c
	tls_config.c
	tls_conninfo.c
	tls_keypair.c
	tls_server.c
	tls_signer.c
	tls_ocsp.c
	tls_peer.c
	tls_util.c
	tls_verify.c
)

if(WIN32)
	set(
		TLS_SRC
		${TLS_SRC}
		compat/ftruncate.c
		compat/pread.c
		compat/pwrite.c
	)
endif()


add_definitions(-DLIBRESSL_INTERNAL)
add_definitions(-DOPENSSL_NO_HW_PADLOCK)
add_definitions(-D__BEGIN_HIDDEN_DECLS=)
add_definitions(-D__END_HIDDEN_DECLS=)

set(CMAKE_POSITION_INDEPENDENT_CODE true)

if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_definitions(-Wno-pointer-sign)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_definitions(-fno-common)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-D_DEFAULT_SOURCE)
    add_definitions(-D_BSD_SOURCE)
    add_definitions(-D_POSIX_SOURCE)
    add_definitions(-D_GNU_SOURCE)
endif()

check_function_exists(asprintf HAVE_ASPRINTF)
if(HAVE_ASPRINTF)
    add_definitions(-DHAVE_ASPRINTF)
endif()

#check_function_exists(reallocarray HAVE_REALLOCARRAY)
#if(HAVE_REALLOCARRAY)
#    add_definitions(-DHAVE_REALLOCARRAY)
#endif()

check_function_exists(freezero HAVE_FREEZERO)
if(HAVE_FREEZERO)
    add_definitions(-DHAVE_FREEZERO)
endif()

check_function_exists(strcasecmp HAVE_STRCASECMP)
if(HAVE_STRCASECMP)
    add_definitions(-DHAVE_STRCASECMP)
endif()

check_function_exists(strlcat HAVE_STRLCAT)
if(HAVE_STRLCAT)
    add_definitions(-DHAVE_STRLCAT)
endif()

check_function_exists(strlcpy HAVE_STRLCPY)
if(HAVE_STRLCPY)
    add_definitions(-DHAVE_STRLCPY)
endif()

check_function_exists(strndup HAVE_STRNDUP)
if(HAVE_STRNDUP)
    add_definitions(-DHAVE_STRNDUP)
endif()

if(WIN32)
    set(HAVE_STRNLEN true)
    add_definitions(-DHAVE_STRNLEN)
else()
    check_function_exists(strnlen HAVE_STRNLEN)
    if(HAVE_STRNLEN)
        add_definitions(-DHAVE_STRNLEN)
    endif()
endif()

check_function_exists(strsep HAVE_STRSEP)
if(HAVE_STRSEP)
    add_definitions(-DHAVE_STRSEP)
endif()

check_function_exists(timegm HAVE_TIMEGM)
if(HAVE_TIMEGM)
    add_definitions(-DHAVE_TIMEGM)
endif()

check_function_exists(arc4random_buf HAVE_ARC4RANDOM_BUF)
if(HAVE_ARC4RANDOM_BUF)
    add_definitions(-DHAVE_ARC4RANDOM_BUF)
endif()

check_function_exists(arc4random_uniform HAVE_ARC4RANDOM_UNIFORM)
if(HAVE_ARC4RANDOM_UNIFORM)
    add_definitions(-DHAVE_ARC4RANDOM_UNIFORM)
endif()

check_function_exists(explicit_bzero HAVE_EXPLICIT_BZERO)
if(HAVE_EXPLICIT_BZERO)
    add_definitions(-DHAVE_EXPLICIT_BZERO)
endif()

check_function_exists(getauxval HAVE_GETAUXVAL)
if(HAVE_GETAUXVAL)
    add_definitions(-DHAVE_GETAUXVAL)
endif()

check_function_exists(getentropy HAVE_GETENTROPY)
if(HAVE_GETENTROPY)
    add_definitions(-DHAVE_GETENTROPY)
endif()

check_symbol_exists(getpagesize unistd.h HAVE_GETPAGESIZE)
if(HAVE_GETPAGESIZE)
    add_definitions(-DHAVE_GETPAGESIZE)
endif()

check_function_exists(getprogname HAVE_GETPROGNAME)
if(HAVE_GETPROGNAME)
    add_definitions(-DHAVE_GETPROGNAME)
endif()

check_function_exists(syslog_r HAVE_SYSLOG_R)
if(HAVE_SYSLOG_R)
    add_definitions(-DHAVE_SYSLOG_R)
endif()

check_function_exists(syslog HAVE_SYSLOG)
if(HAVE_SYSLOG)
    add_definitions(-DHAVE_SYSLOG)
endif()

check_symbol_exists(timespecsub sys/time.h HAVE_TIMESPECSUB)
if(HAVE_TIMESPECSUB)
    add_definitions(-DHAVE_TIMESPECSUB)
endif()

check_function_exists(timingsafe_bcmp HAVE_TIMINGSAFE_BCMP)
if(HAVE_TIMINGSAFE_BCMP)
    add_definitions(-DHAVE_TIMINGSAFE_BCMP)
endif()

check_function_exists(timingsafe_memcmp HAVE_TIMINGSAFE_MEMCMP)
if(HAVE_MEMCMP)
    add_definitions(-DHAVE_MEMCMP)
endif()

check_function_exists(memmem HAVE_MEMMEM)
if(HAVE_MEMMEM)
    add_definitions(-DHAVE_MEMMEM)
endif()

check_include_files(endian.h HAVE_ENDIAN_H)
if(HAVE_ENDIAN_H)
    add_definitions(-DHAVE_ENDIAN_H)
endif()

check_include_files(err.h HAVE_ERR_H)
if(HAVE_ERR_H)
    add_definitions(-DHAVE_ERR_H)
endif()

check_include_files("sys/types.h;arpa/inet.h;netinet/ip.h" HAVE_NETINET_IP_H)
if(HAVE_NETINET_IP_H)
    add_definitions(-DHAVE_NETINET_IP_H)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    # Check if we need -lrt to get clock_gettime on Linux
    check_library_exists(rt clock_gettime "time.h" HAVE_CLOCK_GETTIME)
    if (HAVE_CLOCK_GETTIME)
        set(PLATFORM_LIBS ${PLATFORM_LIBS} rt)
    endif()
else()
    # Otherwise, simply check if it exists
    check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
endif()
if(HAVE_CLOCK_GETTIME)
    add_definitions(-DHAVE_CLOCK_GETTIME)
endif()

check_type_size(time_t SIZEOF_TIME_T)
if(SIZEOF_TIME_T STREQUAL "4")
    set(SMALL_TIME_T true)
    add_definitions(-DSMALL_TIME_T)
    message(WARNING " ** Warning, this system is unable to represent times past 2038\n"
                    " ** It will behave incorrectly when handling valid RFC5280 dates")
endif()
add_definitions(-DSIZEOF_TIME_T=${SIZEOF_TIME_T})

if(WIN32)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/posix_win.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} gettimeofday)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} getuid)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_perror)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_fopen)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_fgets)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_open)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_rename)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_connect)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_close)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_read)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_write)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_getsockopt)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_setsockopt)
endif()

if(NOT HAVE_ASPRINTF)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/bsd-asprintf.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} asprintf)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} vasprintf)
endif()

if(NOT HAVE_FREEZERO)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/freezero.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} freezero)
endif()

if(NOT HAVE_GETPAGESIZE)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/getpagesize.c)
endif()

if(NOT HAVE_GETPROGNAME)
    if(WIN32)
        set(CRYPTO_SRC ${CRYPTO_SRC} compat/getprogname_windows.c)
    elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(CRYPTO_SRC ${CRYPTO_SRC} compat/getprogname_linux.c)
    else()
        set(CRYPTO_SRC ${CRYPTO_SRC} compat/getprogname_unimpl.c)
    endif()
endif()

if(NOT HAVE_REALLOCARRAY)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/reallocarray.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} reallocarray)
endif()

#if(NOT HAVE_RECALLOCARRAY)
#    set(CRYPTO_SRC ${CRYPTO_SRC} compat/recallocarray.c)
#    set(EXTRA_EXPORT ${EXTRA_EXPORT} recallocarray)
#endif()

if(NOT HAVE_STRCASECMP)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/strcasecmp.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} strcasecmp)
endif()

#if(NOT HAVE_STRLCAT)
#    set(CRYPTO_SRC ${CRYPTO_SRC} compat/strlcat.c)
#    set(EXTRA_EXPORT ${EXTRA_EXPORT} strlcat)
#endif()

if(NOT HAVE_STRLCPY)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/strlcpy.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} strlcpy)
endif()

if(NOT HAVE_STRNDUP)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/strndup.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} strndup)
    if(NOT HAVE_STRNLEN)
        set(CRYPTO_SRC ${CRYPTO_SRC} compat/strnlen.c)
        set(EXTRA_EXPORT ${EXTRA_EXPORT} strnlen)
    endif()
endif()

if(NOT HAVE_STRSEP)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/strsep.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} strsep)
endif()

#if(NOT HAVE_STRTONUM)
#    set(CRYPTO_SRC ${CRYPTO_SRC} compat/strtonum.c)
#    set(EXTRA_EXPORT ${EXTRA_EXPORT} strtonum)
#endif()

#if(NOT HAVE_SYSLOG_R)
#    set(CRYPTO_SRC ${CRYPTO_SRC} compat/syslog_r.c)
#endif()

if(NOT HAVE_TIMEGM)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/timegm.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} timegm)
endif()

if(NOT HAVE_EXPLICIT_BZERO)
    if(WIN32)
        set(CRYPTO_SRC ${CRYPTO_SRC} compat/explicit_bzero_win.c)
    else()
        set(CRYPTO_SRC ${CRYPTO_SRC} compat/explicit_bzero.c)
        set_source_files_properties(compat/explicit_bzero.c PROPERTIES COMPILE_FLAGS -O0)
    endif()
    set(EXTRA_EXPORT ${EXTRA_EXPORT} explicit_bzero)
endif()

if(NOT HAVE_ARC4RANDOM_BUF)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/arc4random.c)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/arc4random_uniform.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} arc4random)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} arc4random_buf)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} arc4random_uniform)

    if(NOT HAVE_GETENTROPY)
        if(WIN32)
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_win.c)
        elseif(CMAKE_SYSTEM_NAME MATCHES "AIX")
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_aix.c)
        elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_freebsd.c)
        elseif(CMAKE_SYSTEM_NAME MATCHES "HP-UX")
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_hpux.c)
        elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_linux.c)
        elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_netbsd.c)
        elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_osx.c)
        elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
            set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_solaris.c)
        endif()
        set(EXTRA_EXPORT ${EXTRA_EXPORT} getentropy)
    endif()
endif()

if(NOT HAVE_TIMINGSAFE_MEMCMP)
    set(CRYPTO_SRC ${CRYPTO_SRC} compat/timingsafe_memcmp.c)
    set(EXTRA_EXPORT ${EXTRA_EXPORT} timingsafe_memcmp)
endif()


add_library(tls ${TLS_SRC} ${CRYPTO_SRC})
target_link_libraries(tls OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(tls
    PRIVATE
        .
        include/compat
        ${OPENSSL_INCLUDE_DIR}
    PUBLIC
        include)
