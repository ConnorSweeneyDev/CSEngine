#include "csb.hpp"

#include <filesystem>
#include <format>
#include <string>

int csb_main()
{
  csb::target_name = "CSEngine";
  csb::target_artifact = EXECUTABLE;
  csb::target_linkage = STATIC;
  csb::target_subsystem = CONSOLE;
  csb::target_configuration = RELEASE;
  csb::cxx_standard = CXX20;
  csb::warning_level = W4;
  csb::include_files = csb::files_from("program", {".hpp", ".inl"});
  csb::source_files = csb::files_from("program", {".cpp"});
  if (csb::current_platform == WINDOWS)
    csb::libraries = {"kernel32", "user32",   "shell32",  "gdi32",       "imm32", "comdlg32",
                      "ole32",    "oleaut32", "advapi32", "dinput8",     "winmm", "winspool",
                      "setupapi", "uuid",     "version",  "SDL3-static", "glm"};
  else if (csb::current_platform == LINUX)
    csb::libraries = {"pthread", "dl", "m", "SDL3", "glm"};

  csb::vcpkg_install("2025.08.27");
  csb::subproject_install({{"ConnorSweeneyDev/CSResource", "0.0.0", EXECUTABLE}});

  auto dxc_path = std::filesystem::path("build") / "subproject" / "CSResource" / "build" / "dxc" /
                  (std::string("dxc") + (csb::current_platform == WINDOWS ? ".exe" : ""));
  auto resources = csb::files_from("program", {".vert", ".frag", ".png"});
  std::string command = "CSResource compile";
  for (const auto &resource : resources) command += " " + resource.string();
  command += std::format(" {} build/csresource program/include program/source", dxc_path.string());
  csb::task_run(command, resources, {"program/include/resource.hpp", "program/source/resource.cpp"});

  bool has_resource_header = false;
  for (const auto &file : csb::include_files)
    if (file == "program/include/resource.hpp")
    {
      has_resource_header = true;
      break;
    }
  if (!has_resource_header) csb::include_files.push_back("program/include/resource.hpp");
  bool has_resource_source = false;
  for (const auto &file : csb::source_files)
    if (file == "program/source/resource.cpp")
    {
      has_resource_source = true;
      break;
    }
  if (!has_resource_source) csb::source_files.push_back("program/source/resource.cpp");

  csb::generate_compile_commands();
  csb::clang_format("21.1.1", {"program/include/resource.hpp", "program/source/resource.cpp"});
  csb::compile();
  csb::link();

  return CSB_SUCCESS;
}

CSB_MAIN()
