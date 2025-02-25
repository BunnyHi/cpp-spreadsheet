#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    ValidatePosition(pos);

    if (pos.row >= static_cast<int>(cells_.size())) {
        cells_.resize(pos.row + 1);
    }
    if (pos.col >= static_cast<int>(cells_[pos.row].size())) {
        cells_[pos.row].resize(pos.col + 1);
    }

    if (!cells_[pos.row][pos.col]) {
        cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    }

    cells_[pos.row][pos.col]->Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    return GetCellImpl(pos, *this);
}

CellInterface* Sheet::GetCell(Position pos) {
    return GetCellImpl(pos, *this);
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    return GetCellPtrImpl(pos, *this);
}

Cell* Sheet::GetCellPtr(Position pos) {
    return GetCellPtrImpl(pos, *this);
}


void Sheet::ClearCell(Position pos) {
    ValidatePosition(pos);
    if (pos.row < int(std::size(cells_)) && pos.col < int(std::size(cells_[pos.row]))) {

        if (cells_[pos.row][pos.col]) {
            cells_[pos.row][pos.col]->Clear();

            if (!cells_[pos.row][pos.col]->IsReferenced()) {
                cells_[pos.row][pos.col].reset();
            }
        }
    }
}

Size Sheet::GetPrintableSize() const {
    Size size{ 0, 0 };
    for (int row = 0; row < static_cast<int>(cells_.size()); ++row) {
        for (int col = static_cast<int>(cells_[row].size()) - 1; col >= 0; --col) {
            if (cells_[row][col] && !cells_[row][col]->GetText().empty()) {
                size.rows = std::max(size.rows, row + 1);
                size.cols = std::max(size.cols, col + 1);
                break;
            }
        }
    }
    return size;
}

void Sheet::PrintValues(std::ostream& output) const {
    const auto printableSize = GetPrintableSize();

    for (int currentRow = 0; currentRow < printableSize.rows; ++currentRow) {
        for (int currentCol = 0; currentCol < printableSize.cols; ++currentCol) {
            if (currentCol > 0) {
                output << '\t';
            }

            if (currentCol < static_cast<int>(cells_[currentRow].size())) {
                const auto& cell = cells_[currentRow][currentCol];
                if (cell) {
                    std::visit([&output](const auto& obj) {
                        output << obj;
                        }, cell->GetValue());
                }
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    const auto printableSize = GetPrintableSize();

    for (int currentRow = 0; currentRow < printableSize.rows; ++currentRow) {
        for (int currentCol = 0; currentCol < printableSize.cols; ++currentCol) {
            if (currentCol > 0) {
                output << '\t';
            }

            if (currentCol < static_cast<int>(cells_[currentRow].size())) {
                const auto& cell = cells_[currentRow][currentCol];
                if (cell) {
                    output << cell->GetText();
                }
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}

//---------------------------------Private---------------------------------

void Sheet::ValidatePosition(Position pos) const {
    if (!pos.IsValid()) throw InvalidPositionException("Invalid position");
}

template <typename SheetType>
CellInterface* Sheet::GetCellImpl(Position pos, SheetType& sheet) {
    sheet.ValidatePosition(pos);
    if (static_cast<size_t>(pos.row) >= sheet.cells_.size() ||
        static_cast<size_t>(pos.col) >= sheet.cells_[pos.row].size()) {
        return nullptr;
    }
    return sheet.cells_[pos.row][pos.col].get();
}

template <typename SheetType>
Cell* Sheet::GetCellPtrImpl(Position pos, SheetType& sheet) {
    sheet.ValidatePosition(pos);
    if (static_cast<size_t>(pos.row) >= sheet.cells_.size() ||
        static_cast<size_t>(pos.col) >= sheet.cells_[pos.row].size()) {
        return nullptr;
    }
    return sheet.cells_[pos.row][pos.col].get();
}
