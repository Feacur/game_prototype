" nvim 0.7.0

" this file location:
" - [windows]: `~/.config/nvim` == `%localappdata%/nvim` or `~/AppData/Local/nvim`


" ----- ----- ----- ----- -----
"     early startup commands
" ----- ----- ----- ----- -----

language en
set novisualbell belloff=all

set encoding=utf-8
set fileencoding=utf-8

set hidden
set autoread noautowriteall
set nobackup nowritebackup noswapfile

set completeopt=menu,menuone,longest,noinsert,noselect
set wildmenu wildmode=full
set ignorecase smartcase
set ttyfast lazyredraw
set incsearch hlsearch

set nowrap
syntax enable
filetype plugin indent on
set omnifunc=syntaxcomplete#Complete
set formatoptions=q
set shortmess=filnxtIF

" " native GDB support
" packadd termdebug

if has('win32')
	set shell=cmd
endif

if exists('g:neovide')
	nnoremap <A-CR> <cmd>let g:neovide_fullscreen = !g:neovide_fullscreen<CR>
endif

if has('nvim')
	" @idea: make this config even more safe for vanilla Vim?
	autocmd TermOpen * startinsert
endif

" ----- ----- ----- ----- -----
"     terminal
" ----- ----- ----- ----- -----

set termguicolors

" @note: make sure to install [JetBrains Mono](https://github.com/JetBrains/JetBrainsMono) font
"        or use any other you prefer, obviously
"        [nerd-fonts](https://github.com/ryanoasis/nerd-fonts) contain glyph icons for `guifontwide`
"
set guifont=JetBrains\ Mono:h11

" " defaults seems to be quite convenient
" set guicursor=
" set guicursor+=n-v-c-sm:block
" set guicursor+=i-ci-ve:ver25
" set guicursor+=r-cr-o:hor20

" set guioptions=egmrLT


" ----- ----- ----- ----- -----
"     editor
" ----- ----- ----- ----- -----

set cmdheight=1
set showtabline=0
set signcolumn=yes
set number relativenumber
set list listchars=eol:↓,tab:→\ ,space:•,nbsp:¤

set tabstop=4
set shiftwidth=4

autocmd ColorScheme * highlight Error NONE
autocmd ColorScheme * highlight ErrorMsg NONE


" ----- ----- ----- ----- -----
"     statusline
" ----- ----- ----- ----- -----

if v:true " status line

	set laststatus=3
	set noshowmode noshowcmd noruler

	let g:status_mode={
		\ 	'n'      : 'NORMAL',
		\ 	'i'      : 'INSERT',
		\ 	'v'      : 'VISUAL',
		\ 	'V'      : 'V•LINE',
		\ 	"\<C-v>" : 'V•BLCK',
		\ 	'c'      : 'COMMAND',
		\ 	't'      : 'TERMINAL',
		\ }

	let g:status_fileformat={
		\ 	'dos'  : 'CR/LF',
		\ 	'unix' : 'LF',
		\ 	'mac'  : 'CR',
		\ }

	function g:StatusFileMode()
		return &readonly?'':&modified?'●':'⭘'
	endfunction

	function g:StatusEditorMode()
		return get(g:status_mode,mode(),'['..mode()..']')
	endfunction

	function g:StatusEncoding()
		return &fileencoding?&fileencoding:&encoding
	endfunction

	function g:StatusFormat()
		return get(g:status_fileformat,&fileformat,'??')
	endfunction

	"     

	set statusline=
	set statusline+=┃\ %{g:StatusFileMode()}\ %{g:StatusEditorMode()}\ %f\ 
	set statusline+=%=
	set statusline+=\ %{g:StatusEncoding()}\ %{g:StatusFormat()}\ %y\ %l:%c\ \ %3p%%\ 

	autocmd Filetype qf set statusline=
	autocmd Filetype qf set statusline+=┃\ %{g:StatusFileMode()}\ %{g:StatusEditorMode()}\ %f\ 
	autocmd Filetype qf set statusline+=%=
	autocmd Filetype qf set statusline+=\ %{g:StatusEncoding()}\ %{g:StatusFormat()}\ %y\ %l:%c\ \ %3p%%\ 

endif


" ----- ----- ----- ----- -----
"     controls
" ----- ----- ----- ----- -----

set mouse=a
set scrolloff=8
set whichwrap+=<,>,[,],h,l
set backspace=indent,eol,start

" remap: do not suspend the process
nnoremap <C-z> <NOP>

" easier terminal -> normal transition
tnoremap <leader><esc> <C-\><C-n>

" jump between buffer without completion plugins
nnoremap <leader>b :buffers<CR>:buffer<space>

" resize windows
nnoremap <A-l> <cmd>vertical-resize +1<CR>
nnoremap <A-h> <cmd>vertical-resize -1<CR>
nnoremap <A-k> <cmd>resize +1<CR>
nnoremap <A-j> <cmd>resize -1<CR>

" diagnostics; @todo: contextualize
nnoremap <leader>df <cmd>lua vim.diagnostic.open_float()<CR>
nnoremap <leader>dn <cmd>lua vim.diagnostic.goto_next()<CR>
nnoremap <leader>dp <cmd>lua vim.diagnostic.goto_prev()<CR>

" find in files: current word
nnoremap <leader>cf <cmd>silent grep <cword> \| cwindow<CR>
nnoremap <leader>cn <cmd>cnext<CR>
nnoremap <leader>cp <cmd>cprevious<CR>

nnoremap <leader>lf <cmd>silent lgrep <cword> \| lwindow<CR>
nnoremap <leader>ln <cmd>lnext<CR>
nnoremap <leader>lp <cmd>lprevious<CR>

command ClearQuickFixList cclose | cexpr []
command ClearLocationList lclose | lexpr []


" ----- ----- ----- ----- -----
"     easy mode
" ----- ----- ----- ----- -----

autocmd BufEnter * let b:EasyMode = v:false
function g:EasyMode()
	let b:EasyMode = v:true

	" CR/LF behaviour
	nnoremap <buffer> <leader><CR> a
	nnoremap <buffer> <leader><LF> o

	" normal mode
	nnoremap <buffer> <C-a>      ggVG
	nnoremap <buffer> <CA-left>  <C-o>
	nnoremap <buffer> <CA-right> <C-i>
	nnoremap <buffer> <A-up>     {
	nnoremap <buffer> <A-down>   }
	nnoremap <buffer> <C-s>      <cmd>w<CR>
	nnoremap <buffer> <C-k>s     <cmd>wa<CR>
	nnoremap <buffer> <A-m>      %
	nnoremap <buffer> <A-M>      v%

	" input mode
	inoremap <buffer> <C-a>      <C-o>ggVG
	inoremap <buffer> <CA-left>  <C-o><C-o>
	inoremap <buffer> <CA-right> <C-o><C-i>
	inoremap <buffer> <A-up>     <C-o>{
	inoremap <buffer> <A-down>   <C-o>}
	inoremap <buffer> <C-s>      <C-o><cmd>w<CR>
	inoremap <buffer> <C-k>s     <C-o><cmd>wa<CR>
	inoremap <buffer> <A-m>      <C-o>%
	inoremap <buffer> <A-M>      <C-o>v%

	" [ctrl+]shift selection
	nnoremap <buffer> <S-up>     v<up>
	nnoremap <buffer> <S-down>   v<down>
	nnoremap <buffer> <S-left>   v<left>
	nnoremap <buffer> <S-right>  v<right>
	nnoremap <buffer> <SC-up>    v{
	nnoremap <buffer> <SC-down>  v}
	nnoremap <buffer> <SC-left>  vb
	nnoremap <buffer> <SC-right> ve

	vnoremap <buffer> <S-up>     <up>
	vnoremap <buffer> <S-down>   <down>
	vnoremap <buffer> <S-left>   <left>
	vnoremap <buffer> <S-right>  <right>
	vnoremap <buffer> <SC-up>    {
	vnoremap <buffer> <SC-down>  }
	vnoremap <buffer> <SC-left>  b
	vnoremap <buffer> <SC-right> e

	inoremap <buffer> <S-up>     <esc>v<up>
	inoremap <buffer> <S-down>   <esc>v<down>
	inoremap <buffer> <S-left>   <esc>v<left>
	inoremap <buffer> <S-right>  <esc>v<right>
	inoremap <buffer> <SC-up>    <esc>v{
	inoremap <buffer> <SC-down>  <esc>v}
	inoremap <buffer> <SC-left>  <esc>vb
	inoremap <buffer> <SC-right> <esc>ve

	" insert and command
	noremap! <buffer> <A-backspace> <C-\><C-o>db
	noremap! <buffer> <A-delete>    <C-o>dw

	" clipboard
	nnoremap <buffer> <C-insert> bve"+y
	nnoremap <buffer> <S-insert> "+p

	vnoremap <buffer> <C-insert> "+y
	vnoremap <buffer> <S-insert> "_dh"+p

	inoremap <buffer> <C-insert> <C-o>V"+y
	inoremap <buffer> <S-insert> <C-r>+

	cnoremap <buffer> <S-insert> <C-r>+

	" @todo: toggle them with unmap?

endfunction
command EasyMode call g:EasyMode()


" ----- ----- ----- ----- -----
"     utilities
" ----- ----- ----- ----- -----

" executes `tabnew | terminal cmd` and cleans up
function s:TabTerminal(cmd)
	let l:local = {}

	function local.OnExit(job_id, data, event) closure
		bdelete "" .. l:local.terminal_buffer
	endfun

	tabnew
	call termopen(a:cmd, {'on_exit': l:local.OnExit})
	let l:local.terminal_buffer = bufnr('%')
endfunction


" ----- ----- ----- ----- -----
"     setup internal plugins
" ----- ----- ----- ----- -----

" netrw
let g:netrw_banner = 0
let g:netrw_altv = 1
let g:netrw_browse_split = 4
let g:netrw_winsize = 25
let g:netrw_liststyle = 3
let g:netrw_sort_options = 'i'
let g:netrw_sort_sequence = '[\/]$'
nnoremap <C-e> <cmd>Lexplore!<CR>


" ----- ----- ----- ----- -----
"     plug CLI apps in
" ----- ----- ----- ----- -----

" @note: [online] manuals are likely more verbose than `--help`

" [lazygit](https://github.com/jesseduffield/lazygit)
"     simple terminal UI for git commands
if executable('lazygit')
	command Lazygit call s:TabTerminal('lazygit --path=.')
endif

" [tig](https://github.com/jonas/tig)
"     Text-mode interface for git
if executable('tig')
	command Tig call s:TabTerminal('tig')
endif

" [gitui](https://github.com/extrawurst/gitui)
"     Blazing 💥 fast terminal-ui for git written in rust 🦀
if executable('gitui')
	command Gitui call s:TabTerminal('gitui')
endif

" [ripgrep](https://github.com/BurntSushi/ripgrep)
"     ripgrep recursively searches directories for a regex pattern while respecting your gitignore
if executable('rg')
	set grepprg=rg\ --smart-case\ --vimgrep
endif

" [fd](https://github.com/sharkdp/fd)
"     A simple, fast and user-friendly alternative to 'find'
if executable('fd')
endif

" [bat](https://github.com/sharkdp/bat)
"     A cat(1) clone with wings.
if executable('bat')
endif

" [fzf](https://github.com/junegunn/fzf)
"     🌸 A command-line fuzzy finder
if executable('fzf')
	if executable('fd')
		let $FZF_DEFAULT_COMMAND = 'fd --strip-cwd-prefix --hidden --path-separator=/'
	elseif executable('rg')
		let $FZF_DEFAULT_COMMAND = 'rg --files --smart-case --hidden --path-separator=/'
	endif

	let $BAT_STYLE = 'numbers,header-filesize'
	let $FZF_DEFAULT_OPTS = '--preview="bat --line-range=:64 --color=always {}" --preview-window="hidden" --bind="ctrl-p:toggle-preview"'

	function g:GoToFile() " @todo: reuse TabTerminal
		let l:local = {}

		function local.OnExit(job_id, data, event) closure
			let l:stdout = getbufline(l:local.terminal_buffer, 1, 1)[0]
			bdelete "" .. l:local.terminal_buffer
			if len(l:stdout) != 0
				execute 'edit' l:stdout
			endif
		endfun

		tabnew
		call termopen('fzf', {'on_exit': l:local.OnExit})
		let l:local.terminal_buffer = bufnr('%')
	endfunction

	command GoToFile call g:GoToFile()
	nnoremap <C-p> <cmd>GoToFile<CR>
endif

" @note: compilers and tools
"     [clang, clangd](https://github.com/llvm/llvm-project)
"     [rustc, cargo](https://github.com/rust-lang/rust)
"     [zig](https://github.com/ziglang/zig)
"         hmm, not sure how to debug yet
"     [gcc, gdb, bash](https://github.com/msys2/msys2-installer)
"         `bash` without `WSL` is hella cool
"     [pwsh](https://github.com/PowerShell/PowerShell)
"         the version is newer than the built-in's one
"     [remedybg](https://remedybg.itch.io/remedybg)
"         just works
"


" ----- ----- ----- ----- -----
"     plugins
" ----- ----- ----- ----- -----

if has('win32')
	set noshellslash
endif

" @note: make sure to install [git](https://github.com/git/git)
"        or [git-for-windows](https://github.com/git-for-windows/git)
"

call plug#begin() " install [vim-plug](https://github.com/junegunn/vim-plug)

" colorschemes
" Plug 'morhetz/gruvbox'         " original
Plug 'gruvbox-community/gruvbox' " some colors are a tad off
Plug 'lifepillar/vim-gruvbox8'   " keeps original spirit
Plug 'sainnhe/sonokai'
Plug 'joshdick/onedark.vim'

" Language Sever Protocol
Plug 'neovim/nvim-lspconfig'

" Debug Adapter Protocol
Plug 'puremourning/vimspector'
" Plug 'mfussenegger/nvim-dap'

" editorconfig
Plug 'editorconfig/editorconfig-vim'

" completion
Plug 'hrsh7th/nvim-cmp'
Plug 'hrsh7th/cmp-nvim-lsp'
Plug 'hrsh7th/cmp-nvim-lsp-signature-help'
Plug 'hrsh7th/cmp-nvim-lsp-document-symbol'
Plug 'hrsh7th/cmp-buffer'
Plug 'hrsh7th/cmp-cmdline'
Plug 'hrsh7th/cmp-path'
Plug 'hrsh7th/cmp-calc'
Plug 'hrsh7th/cmp-nvim-lua'
Plug 'hrsh7th/cmp-emoji'
Plug 'hrsh7th/cmp-omni'

" snippets
Plug 'hrsh7th/vim-vsnip'
Plug 'hrsh7th/cmp-vsnip'

" syntax
Plug 'rust-lang/rust.vim'
Plug 'ziglang/zig.vim'

" " conveniences
" Plug 'nvim-lua/plenary.nvim'
" Plug 'nvim-telescope/telescope.nvim'

call plug#end()

" PlugClean " remember to discard unused plugins
" PlugInstall " remember to pull listed plugins

if has('win32')
	set shellslash

	" built-in `:terminal` without args doesn't handle `shellslash` setting
	command TerminalNoArgs call termopen("cmd")
endif


" ----- ----- ----- ----- -----
"    colorschemes
" ----- ----- ----- ----- -----

" let g:gruvbox_italic=1 " for `gruvbox-community/gruvbox`
colorscheme gruvbox8


" ----- ----- ----- ----- -----
"    plugin: vim-vsnip
" ----- ----- ----- ----- -----

" Expand
imap <expr> <C-j>   vsnip#expandable() ? '<Plug>(vsnip-expand)'         : '<C-j>'
smap <expr> <C-j>   vsnip#expandable() ? '<Plug>(vsnip-expand)'         : '<C-j>'

" Expand or jump
imap <expr> <C-l>   vsnip#available(1) ? '<Plug>(vsnip-expand-or-jump)' : '<C-l>'
smap <expr> <C-l>   vsnip#available(1) ? '<Plug>(vsnip-expand-or-jump)' : '<C-l>'

" Jump forward or backward
imap <expr> <tab>   vsnip#jumpable(1)  ? '<Plug>(vsnip-jump-next)'      : '<Tab>'
smap <expr> <tab>   vsnip#jumpable(1)  ? '<Plug>(vsnip-jump-next)'      : '<Tab>'
imap <expr> <S-tab> vsnip#jumpable(-1) ? '<Plug>(vsnip-jump-prev)'      : '<S-Tab>'
smap <expr> <S-tab> vsnip#jumpable(-1) ? '<Plug>(vsnip-jump-prev)'      : '<S-Tab>'

" Select or cut text to use as $TM_SELECTED_TEXT in the next snippet.
" See https://github.com/hrsh7th/vim-vsnip/pull/50
nmap s <plug>(vsnip-select-text)
xmap s <plug>(vsnip-select-text)
nmap S <plug>(vsnip-cut-text)
xmap S <plug>(vsnip-cut-text)


" ----- ----- ----- ----- -----
"    plugin: vimspector
" ----- ----- ----- ----- -----

let g:vimspector_enable_mappings = 'HUMAN'
let g:vimspector_adapters = {
	\ 	"unitydebug": {
	\ 		"name": "unitydebug",
	\ 		"language": ["csharp"],
	\ 		"command": ["unitydebug"],
	\ 		"configuration": { "name": "unity editor" },
	\ 		"attach": { "pidSelect":"none" }
	\ 	},
	\ 	
	\ 	"codelldb": {
	\ 		"name": "codelldb",
	\ 		"language": ["c", "cpp", "rust"],
	\ 		"command": ["codelldb", "--port", "${unusedLocalPort}"],
	\ 		"port": "${unusedLocalPort}",
	\ 		"configuration": { "type": "lldb" }
	\ 	},
	\ 	
	\ 	"opendebugad7 gdb": {
	\ 		"name": "cppdbg",
	\ 		"language": ["c", "cpp"],
	\ 		"command": ["opendebugad7"],
	\ 		"attach": { "pidProperty": "processId", "pidSelect": "ask" },
	\ 		"configuration": {
	\ 			"type": "cppdbg",
	\ 			"MIMode": "gdb",
	\ 			"MIDebuggerPath": "gdb"
	\ 		}
	\ 	}
	\ }
let g:vimspector_configurations = {
	\ }

" for normal mode - the word under the cursor
nmap <Leader>di <Plug>VimspectorBalloonEval
" for visual mode, the visually selected text
xmap <Leader>di <Plug>VimspectorBalloonEval

" @note: debug adapters in use:
"     [unitydebug](https://github.com/Unity-Technologies/vscode-unity-debug), `/releases/tag/Version-3.0.2`
"         just works (tm)
"
"     [codelldb](https://github.com/vadimcn/vscode-lldb), `/releases/tag/v1.7.0`
"         requires strict folder structure [`./adapter`, `./lldb`]
"         doesn't respect system-wide LLVM instance
"
"     [opendebugad7](https://github.com/microsoft/MIEngine), `/releases/tag/v1.8.2`
"         gdb seems to function, lldb - haven't quite set it up yet
"


" ----- ----- ----- ----- -----
"    Lua
" ----- ----- ----- ----- -----

lua << EOF

local lsp_capabilities = vim.lsp.protocol.make_client_capabilities()

-- plugin: nvim-cmp
local nvim_cmp_formatting_kind = {
	Text          = 'αβ',
	Method        = 'μθ',
	Function      = 'ƒŋ',
	Constructor   = '()',
	Field         = '.?',
	Variable      = '01',
	Class         = '<>',
	Interface     = 'ΙΦ',
	Module        = '#m',
	Property      = '→p',
	Unit          = 'un',
	Value         = '42',
	Enum          = 'nm',
	Keyword       = 'kw',
	Snippet       = 'sn',
	Color         = 'cr',
	File          = '[]',
	Reference     = '&*',
	Folder        = '@f',
	EnumMember    = '::',
	Constant      = 'πφ',
	Struct        = '{}',
	Event         = '⚡⚠',
	Operator      = '+=',
	TypeParameter = 'τρ',
}
local nvim_cmp_formatting_menu = {
	nvim_lsp                 = 'lsp',
	nvim_lsp_signature_help  = 'sig',
	nvim_lsp_document_symbol = 'doc',
	vsnip                    = 'snp',
	buffer                   = 'buf',
	calc                     = 'clc',
	cmdline                  = 'cmd',
	path                     = 'pth',
}

local cmp = require('cmp')
cmp.setup({
	snippet = {
		expand = function(args)
			vim.fn['vsnip#anonymous'](args.body)
		end,
	},
	window = {
		-- completion = cmp.config.window.bordered(),
		-- documentation = cmp.config.window.bordered(),
	},
	experimental = {
		native_menu = false,
		ghost_text = true,
	},
	formatting = {
		format = function(entry, vim_item)
			vim_item.kind = nvim_cmp_formatting_kind[vim_item.kind]
			vim_item.menu = nvim_cmp_formatting_menu[entry.source.name]
			return vim_item
		end
	},
	mapping = {
		['<leader><space>'] = cmp.mapping(cmp.mapping.complete(), { 'i' }),
		['<esc>']           = cmp.mapping({
			i = cmp.mapping.abort(),
			c = cmp.mapping.close(),
		}),
		['<CR>']       = cmp.mapping.confirm({ select = true }),
		['<up>']       = cmp.mapping.select_prev_item({ behavior = cmp.SelectBehavior.Insert }),
		['<down>']     = cmp.mapping.select_next_item({ behavior = cmp.SelectBehavior.Insert }),
		['<S-tab>']    = cmp.mapping.select_prev_item({ behavior = cmp.SelectBehavior.Insert }),
		['<tab>']      = cmp.mapping.select_next_item({ behavior = cmp.SelectBehavior.Insert }),
		['<pagedown>'] = cmp.mapping(cmp.mapping.scroll_docs(4), { 'i', 'c' }),
		['<pageup>']   = cmp.mapping(cmp.mapping.scroll_docs(-4), { 'i', 'c' }),
	},
	sources = cmp.config.sources(
		{{ name = 'nvim_lsp' }, { name = 'nvim_lsp_signature_help' }, { name = 'nvim_lua' }},
		{{ name = 'vsnip' }, { name = 'buffer' }},
		{{ name = 'calc' }},
		{{ name = 'path' }},
		{{ name = 'emoji' }},
		{{ name = 'omni' }}
	)
})

cmp.setup.cmdline('/', {
	mapping = cmp.mapping.preset.cmdline(),
	sources = cmp.config.sources(
		{{ name = 'nvim_lsp_document_symbol' }},
		{{ name = 'buffer' }}
	),
})

cmp.setup.cmdline(':', {
	mapping = cmp.mapping.preset.cmdline(),
	sources = cmp.config.sources(
		{{ name = 'cmdline' }, { name = 'path' }}
	)
})

-- plugin: cmp-nvim-lsp
lsp_capabilities = require('cmp_nvim_lsp').update_capabilities(lsp_capabilities)

-- plugin: nvim-lspconfig
local custom_lsp_attach = function(client, buffer)
	-- @todo: curate keymaps per server
	vim.api.nvim_buf_set_keymap(buffer, 'n', '<f12>',      '<cmd>lua vim.lsp.buf.definition()<CR>',  {noremap = true, silent=true})
	vim.api.nvim_buf_set_keymap(buffer, 'n', '<S-f12>',    '<cmd>lua vim.lsp.buf.references()<CR>',  {noremap = true, silent=true})
	vim.api.nvim_buf_set_keymap(buffer, 'n', '<leader>h',  '<cmd>lua vim.lsp.buf.hover()<CR>',       {noremap = true, silent=true})
	vim.api.nvim_buf_set_keymap(buffer, 'n', '<leader>ca', '<cmd>lua vim.lsp.buf.code_action()<CR>', {noremap = true, silent=true})
	vim.api.nvim_buf_set_keymap(buffer, 'n', '<f2>',       '<cmd>lua vim.lsp.buf.rename()<CR>',      {noremap = true, silent=true})

	vim.api.nvim_buf_set_option(buffer, 'omnifunc', 'v:lua.vim.lsp.omnifunc')
end

local lspconfig = require('lspconfig')

-- [clangd](https://github.com/clangd/clangd) or [LLVM](https://github.com/llvm/llvm-project)
local custom_lsp_attach_clangd = function(client, buffer)
	custom_lsp_attach(client, buffer)
	vim.api.nvim_buf_set_keymap(buffer, 'n', '<A-o>', '<cmd>ClangdSwitchSourceHeader()<CR>', {noremap = true, silent=true})
end

lspconfig.clangd.setup({
	capabilities = lsp_capabilities,
	on_attach = custom_lsp_attach_clangd,
	cmd = { 'clangd', '--compile-commands-dir=.', '--header-insertion=never' },
})

-- [omnisharp](https://github.com/OmniSharp/omnisharp-roslyn)
lspconfig.omnisharp.setup({
	capabilities = lsp_capabilities,
	on_attach = custom_lsp_attach,
	cmd = { 'omnisharp', '--languageserver', '--hostPID ' .. tostring(vim.fn.getpid()) },
})

-- [rust-analyzer](https://github.com/rust-lang/rust-analyzer)
lspconfig.rust_analyzer.setup({
	capabilities = lsp_capabilities,
	on_attach = custom_lsp_attach,
	settings = {
		['rust-analyzer'] = {
			assist = {
				importGranularity = 'module',
				importPrefix = 'self',
			},
			cargo = { loadOutDirsFromCheck = true },
			procMacro = { enable = true },
		}
	}
})

-- [zls](https://github.com/zigtools/zls)
--     Zig LSP implementation + Zig Language Server
lspconfig.zls.setup({
	capabilities = lsp_capabilities,
	on_attach = custom_lsp_attach,
})

-- -- conveniences
-- require('telescope').setup({})

EOF


" @note: in case local `.nvimrc` required
"     set exrc secure


" @note: remember to ask for :help if stuck; additionally:
"     issue: [windows] terminal ignores ANSI escape codes
"         set `HKEY_CURRENT_USER\Console\VirtualTerminalLevel` to `0x00000001`
"
"     issue: [windows] terminal defaults to non-UTF-8 encoding
"         set `HKEY_LOCAL_MACHINE\Software\Microsoft\Command Processor\Autorun` to `chcp 65001 > nul`
"
"     issue: gui* options are [partially] disrespected
"         setup terminal as well
"
"     issue: terminal doesn't render certain glyphs
"         oh well
"
"     issue: some keys are reported as correcponding control characters, rendering mapping useless
"         use <A-key> instead
"
"     issue: <C-space> is reported as <C-@>, which is the control character for <nul>
"         use <leader><space> (my <A-space> is mapped by an application)
"
"     issue: `fzf.vim` doesn't highlight syntax
"         [windows]: enable WSL in the Control Panel, then install Ubuntu from the Microsoft Store
"         [windows]: make sure to install `sudo apt instal bat` through `bash` (might be installed as `batcat`)
"         `termguicolors` should be on
"
"     issue: [windows] standard terminal emulators underperform
"         try [alacritty](https://github.com/alacritty/alacritty)
"         something like `alacritty --command nvim` should do it (though configure `alacritty.yml` too)
"         side note: `emacs` counterpart would be `alacritty --command emacs --no-splash --no-window-system`
"         also as of 06.06.22 mouse doesn't function in the `nvim` via alacritty
"         try [wt](https://github.com/microsoft/terminal)
"         try [neovide](https://github.com/neovide/neovide)
"
"     issue: `GDB` doesn't debug `PDB` files
"         oh well
"


" @note: useful commands
"     find in files equivalent: `:grep pattern path | cwindow`
"     probably you want to limit search pattern by a file extension
"     use `<cword>` to search for a word under cursor
"
"     find and replace: :%s/find/replace/g
"
"     hex dump: :%!xxd
"


" @note: some highlights for the future
"     Normal Comment CursorLine CursorColumn Statement PreProc Ignore Conditional Exception Keyword Label
"
