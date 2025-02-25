#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>

class Cell;

class Sheet : public SheetInterface {
public:
    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;
    const Cell* GetCellPtr(Position pos) const;
    Cell* GetCellPtr(Position pos);

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
    void ValidatePosition(Position pos) const;

    template <typename SheetType>
    static CellInterface* GetCellImpl(Position pos, SheetType& sheet);

    template <typename SheetType>
    static Cell* GetCellPtrImpl(Position pos, SheetType& sheet);
};