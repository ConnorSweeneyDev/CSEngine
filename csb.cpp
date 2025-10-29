#include "csb.hpp"

int csb_main()
{
  csb::target_name = "CSEngine";
  csb::target_artifact = EXECUTABLE;
  csb::target_linkage = STATIC;
  csb::target_subsystem = CONSOLE;
  csb::target_configuration = RELEASE;
  csb::cxx_standard = CXX20;
  csb::warning_level = W4;
  csb::include_files = csb::files_from({"program/include"}, {".hpp", ".inl"}, {"program/include/resource.hpp"});
  csb::source_files = csb::files_from({"program/source"}, {".cpp"}, {"program/source/resource.cpp"});
  if (csb::current_platform == WINDOWS)
    csb::libraries = {"kernel32", "user32",   "shell32",  "gdi32",       "imm32", "comdlg32",
                      "ole32",    "oleaut32", "advapi32", "dinput8",     "winmm", "winspool",
                      "setupapi", "uuid",     "version",  "SDL3-static", "glm"};
  else if (csb::current_platform == LINUX)
    csb::libraries = {"c", "m", "pthread", "dl", "SDL3", "glm"};

  csb::vcpkg_install("2025.08.27");
  csb::subproject_install({{"ConnorSweeneyDev/CSResource", "0.0.0", EXECUTABLE}});

  if (csb::current_platform == LINUX) csb::prepend_environment_variable("LD_LIBRARY_PATH", "temp_dxc");
  csb::task_run("CSResource compile [] build/csresource program/include program/source",
                csb::files_from({"program/shader", "program/texture"}, {".vert", ".frag", ".png"}),
                {"program/include/resource.hpp", "program/source/resource.cpp"});

  csb::generate_compile_commands();
  csb::clang_format("21.1.1", {"program/include/resource.hpp", "program/source/resource.cpp"});
  csb::compile();
  csb::link();

  return CSB_SUCCESS;
}

CSB_MAIN()
