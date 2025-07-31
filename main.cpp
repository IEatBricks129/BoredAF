#include <gtkmm.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <filesystem> // For file system operations
#include <fstream>    // For writing files
#include <cstdlib>    // For std::system
#include <cstdio>     // For popen, pclose (Unix-like)
#include <chrono>     // For unique directory name
#include <thread>
#include <mutex>
#include <glibmm/dispatcher.h>
#include <memory>

/**
 * @brief Function to run a shell command and capture its output.
 * @param command The command to run.
 * @return The output of the command.
 */
std::string run_command(const std::string& command) {
    std::string result;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

/**
 * @brief Function to clone a Git repository.
 * @param repo_url The URL of the repository.
 * @param local_dir The local directory to clone into.
 * @return True if the clone was successful, false otherwise.
 */
bool clone_repository(const std::string& repo_url, const std::string& local_dir) {
    std::string command = "git clone --depth 1 " + repo_url + " \"" + local_dir + "\"";
    std::cout << "Cloning " << repo_url << " into " << local_dir << std::endl;
    int result = std::system(command.c_str());
    return result == 0;
}

// Structure to hold project details
struct Project {
    std::string name;    // Display name in the launcher
    std::string type;    // "C++", "HTML"
    std::string path;    // Path to the project's main executable/HTML file after extraction
};

// Global variable to store the extraction target directory
std::filesystem::path g_extraction_target_dir;

// --- Cloning Status Window ---
class CloningStatusWindow : public Gtk::Window {
public:
    struct RepoStatus {
        std::string name;
        Gtk::Label* label;
    };
    CloningStatusWindow(const std::vector<std::string>& repo_names)
    : vbox(Gtk::ORIENTATION_VERTICAL) {
        set_title("Cloning Projects...");
        set_default_size(400, 300);
        set_position(Gtk::WIN_POS_CENTER);
        vbox.set_spacing(10);
        vbox.set_margin_top(20);
        vbox.set_margin_bottom(20);
        vbox.set_margin_start(20);
        vbox.set_margin_end(20);
        add(vbox);
        auto title = Gtk::make_managed<Gtk::Label>("<b>Cloning Repositories</b>");
        title->set_use_markup(true);
        title->set_halign(Gtk::ALIGN_CENTER);
        vbox.pack_start(*title, Gtk::PACK_SHRINK);
        for (const auto& name : repo_names) {
            auto label = Gtk::make_managed<Gtk::Label>(name + ": <span foreground='blue'>Cloning...</span>");
            label->set_use_markup(true);
            label->set_halign(Gtk::ALIGN_START);
            vbox.pack_start(*label, Gtk::PACK_SHRINK);
            repo_status_.push_back({name, label});
        }
        show_all_children();
    }
    void set_status(size_t idx, bool success) {
        if (idx >= repo_status_.size()) return;
        std::string color = success ? "green" : "red";
        std::string msg = success ? "Cloned" : "Failed";
        repo_status_[idx].label->set_markup(repo_status_[idx].name + ": <span foreground='" + color + "'>" + msg + "</span>");
    }
private:
    Gtk::Box vbox;
    std::vector<RepoStatus> repo_status_;
};

class TempDirCleanup {
public:
    TempDirCleanup(const std::filesystem::path& path_to_clean)
        : path_to_clean_(path_to_clean) {}
    ~TempDirCleanup() {
        if (std::filesystem::exists(path_to_clean_)) {
            std::cout << "Cleaning up temporary directory: " << path_to_clean_ << std::endl;
            std::error_code ec;
            std::filesystem::remove_all(path_to_clean_, ec);
            if (ec) {
                std::cerr << "Error cleaning up temporary directory: " << ec.message() << std::endl;
            }
        }
    }
private:
    std::filesystem::path path_to_clean_;
};


/**
 * @brief Main window for the bare-bones Gtkmm application.
 * This window displays a title, project dropdowns, and an exit button.
 */
class MainWindow : public Gtk::Window {
public:
    /**
     * @brief Constructor for MainWindow.
     * @param projects A vector of Project structs to display.
     */
    MainWindow(const std::vector<Project>& projects)
    : vbox(Gtk::ORIENTATION_VERTICAL), // Initialize the vertical box layout
      projects_(projects)              // Store the projects
    {
        // Set window properties
        set_title("Project Launcher");
        set_default_size(600, 700); // Increased size to accommodate text boxes
        set_position(Gtk::WIN_POS_CENTER); // Center the window on the screen
        
        vbox.set_spacing(15); // Add spacing between elements
        vbox.set_margin_start(20); // Add padding to the sides
        vbox.set_margin_end(20);
        vbox.set_margin_top(20);
        vbox.set_margin_bottom(20);

        add(vbox); // Add the vertical box to the window

        // Create and add a title label
        auto title = Gtk::make_managed<Gtk::Label>("<span size='large'><b>Project Launcher</b></span>");
        title->set_use_markup(true); // Enable Pango markup for styling
        title->set_margin_bottom(10);
        title->set_halign(Gtk::ALIGN_CENTER); // Center the title horizontally
        vbox.pack_start(*title, Gtk::PACK_SHRINK); // Pack the title at the start

        // Group projects by type to create dropdowns
        std::map<std::string, std::vector<Project>> grouped_projects;
        for (const auto& proj : projects_) {
            grouped_projects[proj.type].push_back(proj);
        }

        // Adjust window height based on the number of project types (dropdowns)
        // 100 for title/exit buttons, 60 per dropdown
        // set_default_size(400, 100 + (grouped_projects.size() * 60)); // Removed, using fixed size now

        // Create a MenuButton for each project type
        for (auto const& [type, projs_of_type] : grouped_projects) {
            auto menu_button = Gtk::make_managed<Gtk::MenuButton>();
            menu_button->set_label(type + " Projects"); // Label for the dropdown button
            menu_button->set_halign(Gtk::ALIGN_CENTER);
            menu_button->set_size_request(250, 40);

            auto menu = Gtk::make_managed<Gtk::Menu>(); // Create the dropdown menu

            for (const auto& proj : projs_of_type) {
                auto menu_item = Gtk::make_managed<Gtk::MenuItem>(proj.name);
                // Connect signal to launch the project, capturing 'this' to call member function
                menu_item->signal_activate().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::launch_project), proj));
                menu->append(*menu_item); // Add the menu item to the menu
            }
            menu->show_all(); // Show all items in the menu (important for them to be visible)

            menu_button->set_popup(*menu); // Set the menu as the popup for the button
            vbox.pack_start(*menu_button, Gtk::PACK_SHRINK); // Add the menu button to the main layout
        }

        // --- Output Window Button ---
        auto output_btn = Gtk::make_managed<Gtk::Button>("Show Project Output");
        output_btn->set_halign(Gtk::ALIGN_CENTER);
        output_btn->set_size_request(180, 40);
        output_btn->signal_clicked().connect([this]() {
            m_output_window.show();
        });
        vbox.pack_start(*output_btn, Gtk::PACK_SHRINK);

        // Error Label
        auto error_label = Gtk::make_managed<Gtk::Label>("<b>Error Log:</b>");
        error_label->set_use_markup(true);
        error_label->set_halign(Gtk::ALIGN_START);
        vbox.pack_start(*error_label, Gtk::PACK_SHRINK);

        // Error Text View
        m_error_buffer = Gtk::TextBuffer::create();
        m_error_textview.set_buffer(m_error_buffer);
        m_error_textview.set_editable(false);
        m_error_textview.set_wrap_mode(Gtk::WRAP_WORD);

        m_error_scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        m_error_scrolledwindow.add(m_error_textview);
        m_error_scrolledwindow.set_size_request(-1, 150); // Set a fixed height
        vbox.pack_start(m_error_scrolledwindow, Gtk::PACK_EXPAND_WIDGET); // Expand to fill available space
        // --- End Text Boxes ---

        // Create and add an Exit button
        auto exit_btn = Gtk::make_managed<Gtk::Button>("Exit");
        exit_btn->set_halign(Gtk::ALIGN_CENTER); // Center the exit button
        exit_btn->set_size_request(100, 40); // Set a fixed size
        // Connect the Exit button's clicked signal to hide the window, effectively closing the app
        exit_btn->signal_clicked().connect([this]() { hide(); });
        vbox.pack_start(*exit_btn, Gtk::PACK_SHRINK); // Pack the exit button

        // Show all child widgets within the window
        show_all_children();
    }

private:
    Gtk::Box vbox; // The main vertical box for layout
    std::vector<Project> projects_; // Vector to store project data

    // Output window for project output
    class OutputWindow : public Gtk::Window {
    public:
        OutputWindow()
        : m_output_textview(), m_output_buffer(Gtk::TextBuffer::create()) {
            set_title("Project Output");
            set_default_size(600, 300);
            m_output_textview.set_buffer(m_output_buffer);
            m_output_textview.set_editable(false);
            m_output_textview.set_wrap_mode(Gtk::WRAP_WORD);
            m_output_scrolledwindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
            m_output_scrolledwindow.add(m_output_textview);
            add(m_output_scrolledwindow);
            show_all_children();
        }
        void append_to_output(const std::string& text) {
            m_output_buffer->insert_at_cursor(text);
            auto end_iter = m_output_buffer->end();
            m_output_textview.scroll_to(end_iter, 0.0);
        }
        void clear_output() {
            m_output_buffer->set_text("");
        }
    private:
        Gtk::ScrolledWindow m_output_scrolledwindow;
        Gtk::TextView m_output_textview;
        Glib::RefPtr<Gtk::TextBuffer> m_output_buffer;
    };
    OutputWindow m_output_window;

    // Error log members remain in main window
    Gtk::ScrolledWindow m_error_scrolledwindow;
    Gtk::TextView m_error_textview;
    Glib::RefPtr<Gtk::TextBuffer> m_error_buffer;

    /**
     * @brief Appends text to the main output text box.
     * @param text The text to append.
     */
    void append_to_output(const std::string& text) {
        m_output_window.append_to_output(text);
    }

    /**
     * @brief Appends text to the error log text box.
     * @param text The text to append.
     */
    void append_to_error(const std::string& text) {
        m_error_buffer->insert_at_cursor(text);
        // Auto-scroll to end by getting the end iterator
        auto end_iter = m_error_buffer->end();
        m_error_textview.scroll_to(end_iter, 0.0);
    }

    /**
     * @brief Function to launch a project based on its type and path.
     * This function now includes a basic compilation step for C++ projects
     * and redirects output to the text views.
     */
    void launch_project(const Project& project) {
        // Clear previous output/errors
        m_output_window.clear_output();
        m_error_buffer->set_text("");

        m_output_window.show();
        append_to_output("Attempting to launch: " + project.name + " (Type: " + project.type + ", Path: " + project.path + ")\n");

        if (project.type == "HTML") {
            std::string command;
            #ifdef _WIN32
                command = "start \"\" \"" + project.path + "\"";
            #elif __APPLE__
                command = "open \"" + project.path + "\"";
            #else // Linux
                command = "xdg-open \"" + project.path + "\"";
            #endif
            append_to_output("Executing command: " + command + "\n");
            int result = std::system(command.c_str());
            if (result != 0) {
                append_to_error("Error launching HTML project. Command returned: " + std::to_string(result) + "\n");
            } else {
                append_to_output("HTML project launched successfully.\n");
            }
        } else if (project.name.find("Calculator") != std::string::npos) {
            // Launch the embedded calculator GUI
            try {
                // Dynamically load the CalculatorWindow class
                // CalculatorWindow* calc_win = new CalculatorWindow();
                // calc_win->set_transient_for(*this);
                // calc_win->show();
                append_to_output("Calculator GUI launched. (Feature not available: source missing)\n");
            } catch (const std::exception& e) {
                append_to_error(std::string("Error launching calculator: ") + e.what() + "\n");
            }
        } else if (project.type == "C++") {
            // ...existing code for compiling and running other C++ projects...
            std::filesystem::path source_path = project.path;
            std::filesystem::path output_dir = source_path.parent_path();
            std::string executable_name = source_path.stem().string();

            std::string compile_command;
            std::string run_cmd;
            std::filesystem::path executable_path;

            #ifdef _WIN32
                executable_path = output_dir / (executable_name + ".exe");
                compile_command = "g++ \"" + source_path.string() + "\" -o \"" + executable_path.string() + "\" -std=c++17 2>&1";
                run_cmd = "\"" + executable_path.string() + "\" 2>&1";
            #else // Linux and macOS
                executable_path = output_dir / executable_name;
                compile_command = "g++ \"" + source_path.string() + "\" -o \"" + executable_path.string() + "\" -std=c++17 2>&1";
                run_cmd = "\"" + executable_path.string() + "\" 2>&1";
            #endif

            append_to_output("Compiling C++ project: " + compile_command + "\n");

            FILE* pipe_compile = popen(compile_command.c_str(), "r");
            if (!pipe_compile) {
                append_to_error("Error: Could not execute compile command.\n");
                return;
            }
            std::string compile_output;
            char buffer[1024];
            while (fgets(buffer, sizeof(buffer), pipe_compile) != nullptr) {
                compile_output += buffer;
            }
            int compile_result_code = pclose(pipe_compile);
            if (compile_result_code == -1) {
                append_to_error("Error: Failed to close compile pipe.\n");
                return;
            }

            if (compile_result_code == 0) {
                append_to_output("Compilation successful.\n");
                append_to_output("Running C++ project: " + run_cmd + "\n");

                FILE* pipe_run = popen(run_cmd.c_str(), "r");
                if (!pipe_run) {
                    append_to_error("Error: Could not execute run command.\n");
                    return;
                }
                std::string run_output;
                while (fgets(buffer, sizeof(buffer), pipe_run) != nullptr) {
                    run_output += buffer;
                }
                int run_result_code = pclose(pipe_run);
                if (run_result_code == -1) {
                    append_to_error("Error: Failed to close run pipe.\n");
                    return;
                }

                if (run_result_code == 0) {
                    append_to_output("Project output:\n" + run_output + "\n");
                } else {
                    append_to_error("Error running C++ project. Command returned: " + std::to_string(run_result_code) + "\n");
                    append_to_error("Run output (if any):\n" + run_output + "\n");
                }
            } else {
                append_to_error("Error compiling C++ project. Command returned: " + std::to_string(compile_result_code) + "\n");
                append_to_error("Compiler output:\n" + compile_output + "\n");
            }
        } else {
            append_to_error("Unsupported project type for launching: " + project.type + "\n");
        }
    }
};

/**
 * @brief Main function of the application.
 * Initializes Gtkmm, creates the main window, and runs the application.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Application exit code.
 */
int main(int argc, char* argv[]) {
    // Determine the system's temporary directory
    std::filesystem::path base_temp_dir = std::filesystem::temp_directory_path();
    std::string unique_subdir_name = "BareBonesApp_Projects_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                        std::chrono::system_clock::now().time_since_epoch()).count());
    g_extraction_target_dir = base_temp_dir / unique_subdir_name;
    TempDirCleanup cleanup_on_exit(g_extraction_target_dir);

    // List of project repositories to clone
    struct ProjectRepo {
        std::string repo_url;
        std::string local_dir;
    };
    std::vector<ProjectRepo> project_repos = {
        {"https://github.com/IEatBricks129/kerdle.git", "html/kerdle"},
        {"https://github.com/IEatBricks129/ACEDetail.git", "html/ACEDetail"},
        {"https://github.com/IEatBricks129/IEatBricks.git", "html/IEatBricks"}
    };

    // Prepare repo names for status window
    std::vector<std::string> repo_names;
    for (const auto& repo : project_repos) {
        auto pos = repo.local_dir.find_last_of("/");
        std::string name = (pos != std::string::npos) ? repo.local_dir.substr(pos+1) : repo.local_dir;
        repo_names.push_back(name);
    }

    // Create Gtk::Application
    auto app = Gtk::Application::create(argc, argv, "com.example.barebonesapp");

    // Create and show cloning status window
    auto cloning_window = std::make_unique<CloningStatusWindow>(repo_names);
    cloning_window->show();
    while (Gtk::Main::events_pending()) Gtk::Main::iteration();

    if (!std::filesystem::exists(g_extraction_target_dir)) {
        std::filesystem::create_directories(g_extraction_target_dir);
    }

    std::vector<Project> projects;
    for (size_t i = 0; i < project_repos.size(); ++i) {
        const auto& repo = project_repos[i];
        std::filesystem::path repo_target_dir = g_extraction_target_dir / repo.local_dir;
        std::string clone_cmd = "git clone --depth 1 " + repo.repo_url + " \"" + repo_target_dir.string() + "\"";
        std::cout << "Cloning " << repo.repo_url << " into " << repo_target_dir << std::endl;
        int clone_result = std::system(clone_cmd.c_str());
        cloning_window->set_status(i, clone_result == 0);
        while (Gtk::Main::events_pending()) Gtk::Main::iteration();
        if (clone_result != 0) {
            std::cerr << "Error cloning repo: " << repo.repo_url << " (code " << clone_result << ")" << std::endl;
            continue;
        }
        // Detect project type and main file
        std::string type, main_file, name;
        if (repo.local_dir.find("html/") != std::string::npos) {
            type = "HTML";
            main_file = (repo_target_dir / "index.html").string();
            auto pos = repo.local_dir.find_last_of("/");
            name = (pos != std::string::npos) ? repo.local_dir.substr(pos+1) : repo.local_dir;
            if (name == "ACEDetail") name = "Ace Detail (HTML)";
            else if (name == "IEatBricks") name = "My Portfolio Site (HTML)";
            else if (name == "kerdle") name = "Kerdle :) (HTML)";
            else name += " (HTML)";
        } else if (repo.local_dir.find("cpp/") != std::string::npos) {
            type = "C++";
            for (const auto& entry : std::filesystem::recursive_directory_iterator(repo_target_dir)) {
                if (entry.path().extension() == ".cpp") {
                    main_file = entry.path().string();
                    break;
                }
            }
            auto pos = repo.local_dir.find_last_of("/");
            name = (pos != std::string::npos) ? repo.local_dir.substr(pos+1) : repo.local_dir;
            name += " (C++)";
        }
        if (!main_file.empty()) {
            projects.push_back({name, type, main_file});
        }
    }

    // Close cloning status window
    cloning_window->hide();
    while (Gtk::Main::events_pending()) Gtk::Main::iteration();

    // Show main window
    MainWindow window(projects);
    int result = app->run(window);
    return result;
}