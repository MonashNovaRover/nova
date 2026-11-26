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
      ];

      extraConfig = ''
        " Interface
        set number
        set laststatus=2
        set showmatch
        set showbreak=↳
        set cursorlineopt=number

        " Search
        set ignorecase
        set smartcase
        set hlsearch
        set incsearch
        set gdefault

        " Folding
        set foldlevel=99
        set foldmethod=indent

        " Indentation
        set autoindent
        set shiftround
        set copyindent
        set breakindent
        set shiftwidth=2
        set tabstop=2
        set expandtab

        " Navigation
        set scrolloff=8
        set sidescrolloff=8
        set nostartofline

        " Command-line completion
        set wildmenu
        set wildmode=longest:full,full
        set wildignorecase

        " Clipboard and terminal
        set clipboard=unnamedplus
        set mouse=a
        set hidden
        set encoding=utf-8
        set termguicolors

        " Highlighting
        syntax enable
        filetype plugin indent on

        " Colourscheme
        augroup BackgroundOverride
          autocmd!
          autocmd ColorScheme * highlight Normal guibg=#000000 ctermbg=NONE
          autocmd ColorScheme * highlight NormalNC guibg=#000000 ctermbg=NONE
          autocmd ColorScheme * highlight NonText guibg=NONE ctermbg=NONE
        augroup END
        colorscheme catppuccin_mocha

        " Whitespace highlighter
        highlight ExtraWhitespace ctermbg=NONE guibg=#f5e0dc
        match ExtraWhitespace /\s\+$/

        " Performance
        augroup LargeFile
          autocmd!
          autocmd BufReadPost *
            \ if getfsize(expand("%")) >= 1024 * 1024 |
            \ setlocal noswapfile |
            \ setlocal syntax=off |
            \ endif
        augroup END

        " Yazi keybind
        command! Yazi terminal y
        command! Y terminal y
      '';
      defaultEditor = true;
    };
  };
}
