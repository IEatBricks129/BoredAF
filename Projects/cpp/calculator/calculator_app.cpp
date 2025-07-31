// Projects/cpp/calculator/calculator_app.cpp

#include <gtkmm.h>
#include <string>
#include <sstream>
#include <cmath>

class CalculatorWindow : public Gtk::Window {
public:
    CalculatorWindow() {
        set_title("Project Launcher - Calculator");
        set_default_size(350, 250);
        set_position(Gtk::WIN_POS_CENTER);

        vbox.set_spacing(10);
        vbox.set_margin_top(15);
        vbox.set_margin_bottom(15);
        vbox.set_margin_start(15);
        vbox.set_margin_end(15);
        add(vbox);

        entry.set_placeholder_text("Enter expression (e.g. 2+2*3)");
        vbox.pack_start(entry, Gtk::PACK_SHRINK);

        button_box.set_spacing(5);
        vbox.pack_start(button_box, Gtk::PACK_SHRINK);

        // Add buttons for digits and operators
        const std::vector<std::string> buttons = {
            "7", "8", "9", "/",
            "4", "5", "6", "*",
            "1", "2", "3", "-",
            "0", ".", "=", "+"
        };
        for (const auto& label : buttons) {
            auto btn = Gtk::make_managed<Gtk::Button>(label);
            btn->set_size_request(50, 40);
            button_box.pack_start(*btn, Gtk::PACK_SHRINK);
            btn->signal_clicked().connect([this, label]() {
                on_button_clicked(label);
            });
        }

        result_label.set_text("");
        result_label.set_halign(Gtk::ALIGN_START);
        vbox.pack_start(result_label, Gtk::PACK_SHRINK);

        show_all_children();
    }

private:
    Gtk::Box vbox{Gtk::ORIENTATION_VERTICAL};
    Gtk::Entry entry;
    Gtk::Box button_box{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Label result_label;
    std::string current_expr;

    void on_button_clicked(const std::string& label) {
        if (label == "=") {
            std::string expr = entry.get_text();
            double res = 0.0;
            std::string error;
            if (evaluate_expression(expr, res, error)) {
                result_label.set_text("Result: " + std::to_string(res));
            } else {
                result_label.set_text("Error: " + error);
            }
        } else {
            entry.set_text(entry.get_text() + label);
        }
    }

    // Simple expression evaluator (supports +, -, *, /, parentheses, decimals)
    bool evaluate_expression(const std::string& expr, double& result, std::string& error) {
        try {
            result = parse_expression(expr);
            return true;
        } catch (const std::exception& e) {
            error = e.what();
            return false;
        }
    }

    // Recursive descent parser
    double parse_expression(const std::string& expr) {
        std::istringstream in(expr);
        return parse_add_sub(in);
    }
    double parse_add_sub(std::istringstream& in) {
        double left = parse_mul_div(in);
        while (true) {
            char op = in.peek();
            if (op == '+' || op == '-') {
                in.get();
                double right = parse_mul_div(in);
                if (op == '+') left += right;
                else left -= right;
            } else break;
        }
        return left;
    }
    double parse_mul_div(std::istringstream& in) {
        double left = parse_number(in);
        while (true) {
            char op = in.peek();
            if (op == '*' || op == '/') {
                in.get();
                double right = parse_number(in);
                if (op == '*') left *= right;
                else {
                    if (right == 0.0) throw std::runtime_error("Division by zero");
                    left /= right;
                }
            } else break;
        }
        return left;
    }
    double parse_number(std::istringstream& in) {
        while (isspace(in.peek())) in.get();
        if (in.peek() == '(') {
            in.get();
            double val = parse_add_sub(in);
            if (in.get() != ')') throw std::runtime_error("Mismatched parentheses");
            return val;
        }
        double val;
        in >> val;
        if (in.fail()) throw std::runtime_error("Invalid number");
        return val;
    }
};
