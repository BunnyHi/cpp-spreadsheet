#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <functional>
#include <unordered_set>
#include <optional>

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    bool IsReferenced() const;

private:

    class Impl {
    public:

        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const;
        virtual bool IsCacheValid() const { return true; }
        virtual void InvalidateCache() {}
    };

    class EmptyImpl : public Impl {
    public:

        Value GetValue() const override;
        std::string GetText() const override;
    };

    class TextImpl : public Impl {
    public:

        explicit TextImpl(std::string_view text);
        Value GetValue() const override;
        std::string GetText() const override;

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:

        explicit FormulaImpl(std::string text, SheetInterface& sheet);
        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;
        bool IsCacheValid() const override;
        void InvalidateCache() override;

    private:
        std::unique_ptr<FormulaInterface> formula_ptr_;
        mutable std::optional<FormulaInterface::Value> cache_;
        SheetInterface& sheet_;
    };

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<Cell*> dependent_cells_;
    std::unordered_set<Cell*> referenced_cells_;

    void CheckCyclicDependency(const Impl& new_impl);
    void UpdateDependencies();
    void InvalidateCache();
};