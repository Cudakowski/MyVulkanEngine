#!/usr/bin/env bash

set -euo pipefail

# sprawdzanie argumentu
if [ $# -lt 1 ]; then
    echo "Użycie: $0 <NazwaKlasy> [podkatalog]"
    echo "Przykład: $0 MppMaterial"
    echo "Przykład z podkatalogiem: $0 MppShadowSystem rendering"
    exit 1
fi

CLASS_NAME="$1"
SUBDIR="${2:-}"

# sprawdzanie nazwy klasy
if [[ ! "$CLASS_NAME" =~ ^[A-Z][a-zA-Z0-9]*$ ]]; then
    echo "Error: Wymagany format nazwy klasy PascalCase (np. MppRenderer)."
    exit 1
fi

# PascalCase -> snake_case
FILE_BASE=$(echo "$CLASS_NAME" | sed -E 's/([a-z0-9])([A-Z])/\1_\2/g' | tr '[:upper:]' '[:lower:]')

# Ustalenie ścieżek docelowych
if [ -n "$SUBDIR" ]; then
    INC_DIR="include/$SUBDIR"
    SRC_DIR="src/$SUBDIR"
    INCLUDE_PATH="$SUBDIR/${FILE_BASE}.hpp"
else
    INC_DIR="include"
    SRC_DIR="src"
    INCLUDE_PATH="${FILE_BASE}.hpp"
fi

HPP_FILE="$INC_DIR/${FILE_BASE}.hpp"
CPP_FILE="$SRC_DIR/${FILE_BASE}.cpp"

# Sprawdzenie czy skrypt jest uruchamiany z głównego katalogu projektu
if [ ! -d "include" ] || [ ! -d "src" ]; then
    echo "Error: skrypt w złym katalogu projektu (powinien być w głównym)."
    exit 1
fi

# Zabezpieczenie przed nadpisaniem
if [ -f "$HPP_FILE" ] || [ -f "$CPP_FILE" ]; then
    echo "Błąd: Plik $HPP_FILE lub $CPP_FILE już istnieje"
    exit 1
fi

mkdir -p "$INC_DIR" "$SRC_DIR"

# .hpp
cat <<EOF > "$HPP_FILE"
#pragma once

namespace mpp
{

class ${CLASS_NAME}
{
private:
    /* data */
public:
    ${CLASS_NAME}(/* args */);
    ~${CLASS_NAME}();

    //ustaw pod swoje potrzeby
    ${CLASS_NAME}(const ${CLASS_NAME}&) = delete;
    ${CLASS_NAME}& operator=(const ${CLASS_NAME}&) = delete;
    ${CLASS_NAME}(${CLASS_NAME}&&) = delete;
    ${CLASS_NAME}&& operator=(${CLASS_NAME}&&) = delete;
};

} // namespace mpp
EOF

# .cpp
cat <<EOF > "$CPP_FILE"
#include "${INCLUDE_PATH}"

namespace mpp
{

${CLASS_NAME}::${CLASS_NAME}(/* args */)
{
}

${CLASS_NAME}::~${CLASS_NAME}()
{
}


} // namespace mpp
EOF

echo "Utworzono:"
echo "  -> $HPP_FILE"
echo "  -> $CPP_FILE"
echo ""
echo "Dodaj ${SRC_DIR#src/}/${FILE_BASE}.cpp do SOURCES w src/CMakeLists.txt"