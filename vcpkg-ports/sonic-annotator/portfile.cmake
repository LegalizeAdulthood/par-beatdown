set(VERSION "1.7")
set(RELEASE_TAG "sonic-annotator-${VERSION}")
set(BASE_URL "https://github.com/sonic-visualiser/sonic-annotator/releases/download/${RELEASE_TAG}")
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

if(VCPKG_TARGET_IS_WINDOWS AND VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(ARCHIVE_NAME "sonic-annotator-1.7-win64.zip")
    set(SHA512 "a765d86c6927e4f3a84469dc534cbcc19f7a28e4797f13fe9bed961ae88d591556eeec4570f00006f6e2633f5417b2495cc3e65e519365a1195c00c6bb09bd13")
    set(COPYING_FILE "COPYING.txt")
    set(EXE_NAME "sonic-annotator.exe")
elseif(VCPKG_TARGET_IS_LINUX AND VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(ARCHIVE_NAME "sonic-annotator-1.7.0-linux64-static.tar.gz")
    set(SHA512 "476640bc2f672e903ee8d29a81459949a91c55382a07890fa5db3412bb3dde4a153c8f184266206034ec1f568c554dcd96fa6b7c9f915ce84f2e02648d2746f0")
    set(COPYING_FILE "COPYING")
    set(EXE_NAME "sonic-annotator")
elseif(VCPKG_TARGET_IS_OSX)
    set(ARCHIVE_NAME "sonic-annotator-1.7.0-macos.tar.gz")
    set(SHA512 "0b1801f795bb84296858d23e659e1359f47e7721dd3a7644eb3c47c03cea41e572765b481170b0e195f0b495c1636a3a6cbfc7367e427ff883270208e467b58f")
    set(COPYING_FILE "COPYING")
    set(EXE_NAME "sonic-annotator")
else()
    message(FATAL_ERROR "${PORT} only supports x64 Windows, x64 Linux, and macOS.")
endif()

vcpkg_download_distfile(ARCHIVE
    URLS "${BASE_URL}/${ARCHIVE_NAME}"
    FILENAME "${ARCHIVE_NAME}"
    SHA512 "${SHA512}"
)

vcpkg_extract_source_archive(SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    SOURCE_BASE "${PORT}-${VERSION}"
)

file(INSTALL "${SOURCE_PATH}/"
    DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}"
)

if(NOT VCPKG_TARGET_IS_WINDOWS)
    file(CHMOD "${CURRENT_PACKAGES_DIR}/tools/${PORT}/${EXE_NAME}"
        PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
    )
endif()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/${COPYING_FILE}")
file(INSTALL "${CURRENT_PORT_DIR}/usage"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
)
