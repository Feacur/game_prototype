" nvim 0.7.0

" this file location:
" - [windows]: `~/.vim` == `%localappdata%/nvim`


" ----- ----- ----- ----- -----
"     startup commands
" ----- ----- ----- ----- -----
if exists('g:GuiLoaded')
	nnoremap <A-CR> <cmd>call GuiWindowFullScreen(1 - g:GuiWindowFullScreen)<CR>
endif


" @note: remember to ask for :help if stuck; additionally:
" - issue: `nvim-qt` doesn't seem to have built-in fullscreen toggle via <A-CR>
"          ... map one using `GuiWindowFullScreen`
"
" - issue: `nvim-qt` ignores <C-/>
"          ... remap it
"
