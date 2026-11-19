{ config, lib, pkgs, ... }:

let
  cfg = config.nova.user;
in
{
  config = lib.mkIf cfg.enable {
    programs.vim = {
      enable = true;

      plugins = [
        pkgs.vimPlugins.catppuccin-vim
        pkgs.vimPlugins.colorizer
      ];

      extraConfig = ''
        set number
        set ignorecase
        set smartcase
        set showmatch
        set hlsearch
        set incsearch
        set foldlevel=99
        set foldmethod=indent
        set hidden
        set encoding=utf-8
        set termguicolors
        set laststatus=2
        set breakindent
        set showbreak=↳
        set cursorlineopt=number
        set clipboard=unnamedplus
        set mouse=a
        set shiftwidth=2
        set tabstop=2
        set expandtab
        syntax enable
        filetype plugin indent on

        augroup BackgroundOverride
          autocmd!
          autocmd ColorScheme * highlight Normal guibg=#000000 ctermbg=NONE
          autocmd ColorScheme * highlight NormalNC guibg=#000000 ctermbg=NONE
          autocmd ColorScheme * highlight NonText guibg=NONE ctermbg=NONE
        augroup END
        colorscheme catppuccin_mocha

        let g:colorizer_auto_filetype = '*'

        augroup LargeFile
          autocmd!
          autocmd BufReadPre *
            \ if getfsize(expand("%")) > 1024 * 1024 |
            \ setlocal noswapfile |
            \ setlocal syntax=off |
            \ ColorClear |
            \ endif
        augroup END
      '';
      defaultEditor = true;
    };
  };
}
