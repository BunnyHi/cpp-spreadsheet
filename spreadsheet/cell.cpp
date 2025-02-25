#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

//----------------------------------------Impl----------------------------------------
//------Empty
Cell::Value Cell::EmptyImpl::GetValue() const { 
    return "";
}

std::string Cell::EmptyImpl::GetText() const { 
    return "";
}

std::vector<Position> Cell::Impl::GetReferencedCells() const { 
    return {}; 
}

//------Text
Cell::TextImpl::TextImpl(std::string_view text)
    : text_(std::move(text)) {}

Cell::Value Cell::TextImpl::GetValue() const {
    if (text_.empty()) {
        throw FormulaException("it is empty impl, not text");

    }
    else if (text_.at(0) == ESCAPE_SIGN) {
        return text_.substr(1);

    }
    else {
        return text_;
    }
}

std::string Cell::TextImpl::GetText() const { 
    return text_;
}

//------Formula
Cell::FormulaImpl::FormulaImpl(std::string text, SheetInterface& sheet)
    : formula_ptr_(ParseFormula(text.substr(1)))
    , sheet_(sheet) {
}

Cell::Value Cell::FormulaImpl::GetValue() const {
    if (!cache_) cache_ = formula_ptr_->Evaluate(sheet_);

    auto value = formula_ptr_->Evaluate(sheet_);
    if (std::holds_alternative<double>(value)) return std::get<double>(value);

    return std::get<FormulaError>(value);
}

std::string Cell::FormulaImpl::GetText() const { 
    return FORMULA_SIGN + formula_ptr_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const { 
    return formula_ptr_->GetReferencedCells(); 
}

bool Cell::FormulaImpl::IsCacheValid() const{
    return cache_.has_value();
}

void Cell::FormulaImpl::InvalidateCache(){
    cache_.reset();
}

//----------------------------------------Impl----------------------------------------

//----------------------------------------Cell----------------------------------------
Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {
}

Cell::~Cell() {}

void Cell::CheckCyclicDependency(const Impl& new_impl) {
    const auto referenced_positions = new_impl.GetReferencedCells();

    if (referenced_positions.empty()) {
        return;
    }

    std::unordered_set<const Cell*> referenced_cells;
    for (const auto& position : referenced_positions) {
        referenced_cells.insert(sheet_.GetCellPtr(position));
    }

    std::unordered_set<const Cell*> visited_cells;
    std::vector<const Cell*> cells_to_visit = { this };

    while (!cells_to_visit.empty()) {
        const Cell* current_cell = cells_to_visit.back();
        cells_to_visit.pop_back();
        visited_cells.insert(current_cell);

        if (referenced_cells.count(current_cell) > 0) {
            throw CircularDependencyException("");
        }

        for (const Cell* dependent_cell : current_cell->dependent_cells_) {
            if (visited_cells.insert(dependent_cell).second) {
                cells_to_visit.push_back(dependent_cell);
            }
        }
    }
}

void Cell::UpdateDependencies() {
    for (Cell* referenced_cell : referenced_cells_) {
        referenced_cell->dependent_cells_.erase(this);
    }

    referenced_cells_.clear();

    const auto& new_referenced_positions = impl_->GetReferencedCells();

    for (const auto& position : new_referenced_positions) {
        Cell* referenced_cell = sheet_.GetCellPtr(position);
        if (!referenced_cell) {
            sheet_.SetCell(position, "");
            referenced_cell = sheet_.GetCellPtr(position);
        }
        referenced_cells_.insert(referenced_cell);
        referenced_cell->dependent_cells_.insert(this);
    }
}

void Cell::InvalidateCache() {
    if (impl_->IsCacheValid()) {
        impl_->InvalidateCache();

        for (Cell* dependent : dependent_cells_) {
            dependent->InvalidateCache();
        }
    }
}

void Cell::Set(std::string text) {
    std::unique_ptr<Impl> impl;
    if (text.empty()) {
        impl = std::make_unique<EmptyImpl>();
    }
    else if (text[0] == FORMULA_SIGN) {
        impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);
    }
    else {
        impl = std::make_unique<TextImpl>(std::move(text));
    }      

    CheckCyclicDependency(*impl);

    impl_ = std::move(impl);

    UpdateDependencies();
    InvalidateCache();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const { 
    return impl_->GetReferencedCells(); 
}

bool Cell::IsReferenced() const { 
    return !dependent_cells_.empty();
}

//----------------------------------------Cell----------------------------------------