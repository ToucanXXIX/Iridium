{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  packages = with pkgs; [
    cmake
    clang
    ccache
    pkg-config
    python3
    
    libGL
    vulkan-headers
    vulkan-loader
    vulkan-tools
    vulkan-validation-layers
    vulkan-extension-layer
    glslang
    wayland-scanner

    wayland
    libxkbcommon
    libx11
    libxrandr
    libxinerama
    libxcursor
    libxi
  ];

  LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath [
    pkgs.wayland
    pkgs.libxkbcommon
    pkgs.vulkan-loader
  ];
  
  VK_LAYER_PATH = pkgs.lib.concatStringsSep ":" [
    "${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
    "${pkgs.vulkan-extension-layer}/share/vulkan/explicit_layer.d"
  ];
}

