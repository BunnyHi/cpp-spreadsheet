#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try : ast_(ParseFormulaAST(expression)) {}
    catch (const std::exception& e) {
        throw FormulaException("Failed to parse formula: " + std::string(e.what()));
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> args = [&sheet](const Position p)->double {
            if (!p.IsValid()) throw FormulaError(FormulaError::Category::Ref);

            const auto* cell = sheet.GetCell(p);
            if (!cell) return 0;

            if (std::holds_alternative<double>(cell->GetValue())) {
                return std::get<double>(cell->GetValue());
            }

            if (std::holds_alternative<std::string>(cell->GetValue())) {
                auto value = std::get<std::string>(cell->GetValue());
                double result = 0;

                if (!value.empty()) {
                    std::istringstream in(value);
                    if (!(in >> result) || !in.eof()) throw FormulaError(FormulaError::Category::Value);
                }

                return result;
            }
            throw FormulaError(std::get<FormulaError>(cell->GetValue()));
        };

        try {
            return ast_.Execute(args);
        }
        catch (const FormulaError& formula_error) {
            return formula_error;
        }
    }

    std::vector<Position> GetReferencedCells() const override {
        std::set<Position> unique_elements(ast_.GetCells().begin(), ast_.GetCells().end());
        return { unique_elements.begin(), unique_elements.end() };
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}