#!/bin/bash

NAME="vlc-discordrpc-plugin"
RELEASES_DIR="./releases"
CURRENT_VERSION="1.2.0"

mkdir -p "${RELEASES_DIR}/fedora"
mkdir -p "${RELEASES_DIR}/ubuntu"

echo "--- Building and extracting for Fedora ---"
docker build -t "${NAME}-fedora" -f Dockerfile.fedora --build-arg VERSION="${CURRENT_VERSION}" --build-arg NAME="${NAME}" --output "${RELEASES_DIR}/fedora" .

echo "--- Building and extracting for Ubuntu ---"
docker build -t "${NAME}-ubuntu" -f Dockerfile.ubuntu --build-arg VERSION="${CURRENT_VERSION}" --build-arg NAME="${NAME}" --output "${RELEASES_DIR}/ubuntu" .

echo "--------------------------------------------------------"
echo "Process completed. Binaries are in ${RELEASES_DIR}"
