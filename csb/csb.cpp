#include "csb.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <unordered_map>
#include <vector>

void csb::configure()
{
  csb::target_name = "cse";
  csb::target_artifact = STATIC_LIBRARY;
  csb::target_linkage = STATIC;
  csb::target_configuration = RELEASE;
  csb::cxx_standard = CXX20;
  csb::warning_level = W4;
  csb::include_files = csb::choose_files({"program/include"}, {}, {"program/include/shader.hpp"});
  csb::source_files = csb::choose_files({"program/source"}, {}, {"program/source/shader.cpp"});
}

int csb::clean()
{
  csb::clean_build();
  csb::clean({"program/include/shader.hpp", "program/source/shader.cpp"});
  return csb::build();
}

int csb::build()
{
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
                                       {
                                         {"name", "sdl3-mixer"},
                                         {"default-features", false},
                                         {"features",
                                          {
                                            "opusfile",
                                          }},
                                       },
                                       "glm",
                                       "nlohmann-json",
                                     }},
                                    {"overrides",
                                     {
                                       {
                                         {"name", "sdl3"},
                                         {"version", "3.4.4"},
                                       },
                                       {
                                         {"name", "sdl3-mixer"},
                                         {"version", "3.2.0#2"},
                                       },
                                       {
                                         {"name", "glm"},
                                         {"version", "1.0.1#3"},
                                       },
                                       {
                                         {"name", "nlohmann-json"},
                                         {"version", "3.12.0#2"},
                                       },
                                     }}});

  csb::subproject_install({"ConnorSweeneyDev/CSData", "1.0.0", HEADER_LIBRARY});
  csb::subproject_install({"ConnorSweeneyDev/CSPack", "1.0.0", HEADER_LIBRARY});

  csb::generate_clang_format({{"BasedOnStyle", "LLVM"},
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
  if (!csb::is_subproject)
    csb::format("22.1.8", csb::choose_files({"program/vertex", "program/fragment"}),
                {"program/include/shader.hpp", "program/source/shader.cpp"});

  csb::archive_install(
    {csb::host_platform == WINDOWS
       ? "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2602.24/dxc_2026_05_27.zip"
       : "https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.9.2602.24/"
         "linux_dxc_2026_05_26.x86_64.tar.gz",
     "build/dxc",
     {csb::host_platform == WINDOWS ? "bin/" + csb::host_architecture : "bin",
      csb::host_platform == WINDOWS ? "lib/" + csb::host_architecture : "lib"},
     {}});
  if (csb::host_platform == LINUX)
  {
    csb::multi_task_run("chmod +x ()",
                        csb::choose_files({"build/dxc"}, [](const auto &file) { return file.extension() == ""; }),
                        {"build/dxc/executable.(filename)"});
    csb::prepend_environment_variable("LD_LIBRARY_PATH", "build/dxc");
  }
  std::unordered_map<std::filesystem::path, std::vector<std::filesystem::path>> shader_groups{};
  for (const auto &file : csb::choose_files({"program/vertex", "program/fragment"}, [](const auto &file)
                                            { return file.extension() == ".vert" || file.extension() == ".frag"; }))
    shader_groups[file.parent_path()].push_back(file);
  for (const auto &[source, group] : shader_groups)
    csb::multi_task_run(
      [](const std::filesystem::path &file, const auto &, const auto &) -> std::string
      {
        return std::format("{} -spirv -T {}_6_0 -E main () -Fo []",
                           csb::host_platform == WINDOWS ? "build\\dxc\\dxc.exe" : "./build/dxc/dxc",
                           file.extension() == ".vert" ? "vs" : "ps");
      },
      group, {(csb::path("build") / source.lexically_relative("program") / "(filename).spv").string()});
  for (const auto &compiled : csb::choose_files({"build/vertex", "build/fragment"}))
    if (!csb::exists(csb::path("program") / compiled.lexically_relative("build").parent_path() / compiled.stem()))
      csb::remove(compiled);

  csb::embed(
    {"// This file is automatically generated, do not edit manually.\n\n"
     "#pragma once\n\n"
     "#include <span>\n\n"
     "namespace cse::shader\n{\n",
     "// This file is automatically generated, do not edit manually.\n\n"
     "#include \"shader.hpp\"\n\n"
     "#include <array>\n"
     "#include <span>\n\n"
     "namespace cse::shader\n{\n"},
    {[](const std::filesystem::path &, const std::string &name, const auto &) -> std::string
     { return std::format("  extern const std::span<const unsigned char> {};\n", name); },
     [](const std::filesystem::path &, const std::string &name, const auto &data) -> std::string
     {
       return std::format("  static constexpr std::array<unsigned char, {}> {}_data{{\n    (0)}};\n"
                          "  const std::span<const unsigned char> {}{{{}_data}};\n",
                          std::get<0>(data).size(), name, name, name);
     },
     [](const std::filesystem::path &file) -> std::string { return file.parent_path().filename().string(); },
     {},
     {}},
    {[](const auto &) -> std::string { return "}\n"; }, [](const auto &) -> std::string { return "}\n"; }}, {},

    csb::choose_files({"build/vertex", "build/fragment"}, [](const auto &file) { return file.extension() == ".spv"; }),
    {"program/include/shader.hpp", "program/source/shader.cpp"}, {});

  const auto build_include_path = csb::path("build/include");
  const auto cse_include_path = build_include_path / "cse";
  if (!csb::exists(cse_include_path)) csb::mkdir(cse_include_path);
  for (const auto &file : csb::directory(cse_include_path))
    if (!csb::contains(csb::include_files, csb::path("program/include") / file.path().filename()))
      csb::remove(file.path());
  csb::multi_task_run(std::format("{} () []", csb::host_platform == WINDOWS ? "copy /Y" : "cp"), csb::include_files,
                      {cse_include_path / "(filename)"});

  std::vector<std::filesystem::path> external_includes{};
  for (const auto &entry : csb::directory(csb::vcpkg_include())) external_includes.push_back(entry.path());
  for (const auto &entry : csb::directory(csb::subproject_include("CSData"))) external_includes.push_back(entry.path());
  for (const auto &entry : csb::directory(csb::subproject_include("CSPack"))) external_includes.push_back(entry.path());
  csb::multi_task_run(
    [](const std::filesystem::path &source, const std::vector<std::filesystem::path> &,
       const std::vector<std::filesystem::path> &) -> std::string
    {
      if (csb::host_platform == WINDOWS)
        return std::filesystem::is_directory(source) ? "xcopy /E /I /Y /Q () []" : "copy /Y () []";
      return std::filesystem::is_directory(source) ? "cp -rT () []" : "cp () []";
    },
    external_includes, {build_include_path / "(filename)"});

  const auto build_library_path{csb::path("build") / (csb::target_configuration == RELEASE ? "release" : "debug")};
  if (!csb::exists(build_library_path)) csb::mkdir(build_library_path);
  std::vector<std::filesystem::path> external_libraries{};
  for (const auto &entry : csb::directory_recurse(csb::vcpkg_library()))
    if (entry.is_regular_file() && entry.path().extension() == (csb::host_platform == WINDOWS ? ".lib" : ".a"))
      external_libraries.push_back(entry.path());
  csb::multi_task_run(std::format("{} () []", csb::host_platform == WINDOWS ? "copy /Y" : "cp"), external_libraries,
                      {build_library_path / "(filename)"});

  csb::generate_compile_commands();
  csb::generate_clangd({{"Diagnostics", {{"UnusedIncludes", "Strict"}, {"MissingIncludes", "Strict"}}}});

  csb::compile();
  csb::link();
  return csb::run();
}

int csb::run() { return csb::success; }

CSB_MAIN()
