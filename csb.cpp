#include "csb.hpp"

#include <format>
#include <string>

void csb::configure()
{
  csb::target_name = "cse";
  csb::target_artifact = STATIC_LIBRARY;
  csb::target_linkage = STATIC;
  csb::target_configuration = RELEASE;
  csb::cxx_standard = CXX20;
  csb::warning_level = W4;
  csb::include_files = csb::choose_files({"program/include"});
  csb::source_files = csb::choose_files({"program/source"});
}

int csb::clean()
{
  csb::clean_build();
  return csb::build();
}

int csb::build()
{
  if (!csb::is_subproject)
    csb::clang_format("22.1.5", {{"BasedOnStyle", "LLVM"},
                                 {"ColumnLimit", "120"},
                                 {"IndentWidth", "2"},
                                 {"ConstructorInitializerIndentWidth", "2"},
                                 {"ContinuationIndentWidth", "2"},
                                 {"Language", "Cpp"},
                                 {"BreakBeforeBraces", "Allman"},
                                 {"AllowShortBlocksOnASingleLine", "true"},
                                 {"AllowShortIfStatementsOnASingleLine", "true"},
                                 {"AllowShortCaseLabelsOnASingleLine", "true"},
                                 {"AllowShortLoopsOnASingleLine", "true"},
                                 {"AllowShortFunctionsOnASingleLine", "true"},
                                 {"AllowShortLambdasOnASingleLine", "true"},
                                 {"AllowShortEnumsOnASingleLine", "true"},
                                 {"AllowShortNamespacesOnASingleLine", "true"},
                                 {"BreakTemplateDeclarations", "No"},
                                 {"IndentPPDirectives", "BeforeHash"},
                                 {"IndentCaseLabels", "true"},
                                 {"NamespaceIndentation", "All"},
                                 {"FixNamespaceComments", "false"}});

  csb::vcpkg_install("2026.04.27", {{"builtin-baseline", "56bb2411609227288b70117ead2c47585ba07713"},
                                    {"dependencies",
                                     {
                                       {
                                         {"name", "sdl3"},
                                         {"default-features", false},
                                         {"features",
                                          {
                                            "vulkan",
                                            {{"name", "x11"}, {"platform", "linux"}},
                                            {{"name", "wayland"}, {"platform", "linux"}},
                                          }},
                                       },
                                       "glm",
                                     }},
                                    {"overrides",
                                     {
                                       {
                                         {"name", "sdl3"},
                                         {"version", "3.4.4"},
                                       },
                                       {
                                         {"name", "glm"},
                                         {"version", "1.0.1#3"},
                                       },
                                     }}});

  auto build_include_path = csb::path("build/include/cse");
  if (!csb::exists(build_include_path)) csb::mkdir(build_include_path);
  for (const auto &file : csb::directory(build_include_path))
    if (!csb::contains(csb::include_files, csb::path("program/include") / file.path().filename()))
      csb::remove(file.path());
  csb::multi_task_run(std::format("{} () []", csb::host_platform == WINDOWS ? "copy /Y" : "cp"), csb::include_files,
                      {build_include_path / "(filename)"});

  csb::generate_clangd({{"Diagnostics", {{"UnusedIncludes", "Strict"}, {"MissingIncludes", "Strict"}}}});
  csb::generate_compile_commands();
  csb::compile();
  csb::link();
  return csb::run();
}

int csb::run() { return csb::success; }

CSB_MAIN()
