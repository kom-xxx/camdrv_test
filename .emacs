;;; -*-emacs-lisp-*-
(require 'cl-lib)
(require 'info)

;;; window
(tool-bar-mode 0)
(menu-bar-mode 0)
(scroll-bar-mode 0)
(show-paren-mode 1)
(normal-erase-is-backspace-mode 0)

(defun string-to-int (string)
  (string-to-number string))

(mapc #'(lambda (x) (cl-pushnew x default-frame-alist))
      '((width . 80) (height . 80)))
(when (getenv "SSH_CLIENT")
  (cl-pushnew
   '(font . "-*-DejaVu Sans Mono-normal-normal-normal-*-12-*-*-*-m-*-*-*")
   default-frame-alist))
	   
;;;; FONT on LAPTOP-KPTBO449(china)
;;;; "-*-DejaVu Sans Mono-normal-normal-normal-*-12-*-*-*-m-0-iso10646-1"
;;; options
(put 'narrow-to-region 'disabled nil)
(put 'upcase-region 'disabled nil)
(put 'downcase-region 'disabled nil)
(put 'set-goal-column 'disabled nil)
(put 'eval-expression 'disabled nil)

(setq inhibit-splash-screen t)
(setq eval-expression-print-level nil)
(setq eval-expression-print-length nil)
(setq scroll-conservatively 1)
(setq warning-suppress-types '((undo discard-info)))
(setq auto-save-list-file-prefix "~/.emacs.d/auto-saves/")
(setq-default case-fold-search nil)

(mapc #'(lambda (x) (cl-pushnew x Info-directory-list))
      '("~/.sbcl/share/info"))

(mapc #'(lambda (x) (cl-pushnew x load-path))
      '("~/.elisp" "~/.elisp/share/emacs/site-lisp" "~/.elisp/slime"
	"~/.elisp/slime/contrib" "~/.elisp/w3m" "~/.elisp/haskell-mode"
	"~/src/acl2-8.0/emacs" "~/.elisp/imaxima"))

;;; 
(global-set-key "\M-," 'pop-tag-mark)
(global-set-key "\M-*" 'tags-loop-continue)

;;; face
(set-face-foreground 'minibuffer-prompt "LightGoldenrod")
(set-face-foreground 'error "brown1")
(set-face-foreground 'font-lock-builtin-face "turquoise3")
(set-face-foreground 'font-lock-comment-face "snow4")
(set-face-foreground 'font-lock-preprocessor-face "DarkCyan")
(set-face-foreground 'font-lock-string-face "SpringGreen3")
(set-face-foreground 'font-lock-function-name-face "SeaGreen2")
(set-face-foreground 'font-lock-variable-name-face "LightGoldenrod2")
(set-face-foreground 'font-lock-keyword-face "DarkSlateGray3")
(set-face-foreground 'font-lock-type-face "PaleGreen3")
(set-face-foreground 'font-lock-warning-face "DarkGoldenrod1")
;(set-face-foreground 'font-lock-doc-face "DarkOrange4")
(set-face-background 'region "sea green")

;; Language system
(set-language-environment 'Japanese)
;(coding-system-put 'cp932 :encode-translation-table
;		   (get 'japanese-ucs-jis-to-cp932-map 'translation-table))
(require 'mozc)
(setq default-input-method "japanese-mozc")
(set-default-coding-systems 'utf-8)
(modify-coding-system-alist 'file "\\.utf8\\.jp\\'" 'utf-8)

;;; tramp
(progn
  (require 'tramp)
  (setf tramp-default-method "rsh")
  (setf tramp-login-program "/usr/local/bin/rlogin")
  (setf tramp-login-args '("-l" "%u"))
  (setf tramp-copy-program "/usr/local/bin/rcp"))

;;; pre package initialize
;(setq swank-clojure-enable nil)

;;; rtags
;(require 'rtags)

(require 'package)
(setq package-archives
      '(("gnu"   . "http://elpa.gnu.org/packages/")
        ("melpa" . "http://melpa.org/packages/")
        ("org"   . "http://orgmode.org/elpa/")))
(package-initialize)

;;; package initialize
;(load "google-c-style")
;(load "init-gtags")
(load "init-auctex")
(load "init-browse-url")
(load "init-cc-mode")
(load "init-comint")
(load "init-eshell")
(load "init-haskell")
(load "init-imaxima")
(load "init-python")
(load "init-sh")
(load "init-slime")
(load "init-verilog")

;(setf lsp-clients-clangd-executable "clangd90")
;;; post package initialize
;(add-hook 'c++-mode-hook 'google-set-c-style)

;;; java
(add-hook 'java-mode-hook
	  (function (lambda ()
		      (setq tab-width 4)
		      (setq c-basic-offset 4))))

;;; GDB mode
;(add-hook 'gdb-mode-hook
;	  (function (lambda ()
;		      (set-buffer-process-coding-system 'euc-jp 'euc-jp))))


(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(w3m-italic ((t (:underline t :slant normal)))))

(require 'bison-mode)
(require 'flex-mode)
;;;;
;;(autoload 'bison-mode "bison-mode")
;;;; *.y *.yy ファイルを 自動的に bison-mode にする
;;(setq auto-mode-alist
;;     (cons '("\.\(y\|yy\)$" . bison-mode) auto-mode-alist))
;;(autoload 'flex-mode "flex-mode")
;;;; *.l *.ll ファイルを 自動的に flex-mode にする
;;(setq auto-mode-alist
;;     (cons '("\.\(l\|ll\)$" . flex-mode) auto-mode-alist))
;;

(require 'gud-lldb)

(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(menu-bar-mode nil)
 '(package-selected-packages '(clang-format))
 '(safe-local-variable-values
   '((indent-tabs-mode . 1)
     (Package . FLEXI-STREAMS)
     (Package . CL-PPCRE)
     (Readtable . PY-AST-READTABLE)
     (Package . CLPYTHON\.UTIL)
     (Package . CLPYTHON\.MODULE\.THREAD)
     (Package . CLPYTHON\.MODULE\.SYS)
     (Package . CLPYTHON\.MODULE\.SYMBOL)
     (Package . CLPYTHON\.MODULE\.STRING)
     (Package . CLPYTHON\.MODULE\.RE)
     (Package . CLPYTHON\.PARSER)
     (readtable . py-user-readtable)
     (package . clpython)
     (Readtable . PY-USER-READTABLE)
     (READTABLE . PY-AST-USER-READTABLE)
     (Readtable . PY-AST-USER-READTABLE)
     (Package . CLPYTHON)
     (Syntax . Ansi-Common-Lisp)
     (Package . page-utils)
     (Package . web-utils)
     (Package . edit-msdosfs)
     (Package . CL-WHO)
     (eval add-hook 'write-file-hooks 'nuke-trailing-whitespace)
     (eval add-hook 'write-file-hooks 'time-stamp)
     (c-continued-brace-offset . 0)
     (c-continued-statement-offset . 2)
     (c-label-offset . -2)
     (c-argdecl-indent . 2)
     (c-brace-offset . -2)
     (c-brace-imaginary-offset . 0)
     (TeX-auto-save . t)
     (TeX-parse-self . t)
     (Package . LOGIN-PAGE)
     (Package . bind)
     (Package . LIFT)
     (Package . User)
     (base . 10)
     (base . 16)
     (package . GETOPT-LIB)
     (Package . JS-JENERATOR)
     (syntax . common-lisp)
     (Package . getopt-tests)
     (Package . getopt)
     (Package . PS-FUNCS)
     (indent-tabs-mode . s)
     (Package . BAYESIAN)
     (Package . HUNCHENTOOT-UTILS)
     (Package . CL-WHO-COMPANION)
     (Package . IPTABLES)
     (Package . CCL)
     (Package . TIMER)
     (Package . CRON)
     (Package . YSM-DETAIL)
     (Package . elephant-test)
     (Syntax . ansi-common-lisp)
     (Package . ja-string)
     (Package . UTIL)
     (Package . PAGE-UTILS)
     (Package . TBNL-UTILS)
     (Package . SHELL)
     (syntax . ANSI-COMMON-LISP)
     (Package . CHUNGA)
     (Package . HUNCHENTOOT)
     (Package . CLIM-CLX)
     (Package . DB)
     (Package SERIES :use "COMMON-LISP" :colon-mode :external)
     (Package . ESTIMATE)
     (Package . YSM-DS)
     (Package . YSM-WEB)
     (Package . cxml)
     (Package . RUNES)
     (Package . SAX)
     (show-trailing-whitespace . t)
     (indent-tabs)
     (Syntax . Common-Lisp)
     (Package . CXML)
     (readtable . runes)
     (Encoding . utf-8)
     (Package . qu-int)
     (Package . dyna)
     (Package . dyna-system)
     (Syntax . COMMON-LISP)
     (Package . dyna-core)
     (package . dyna-xmlparse)
     (package . net\.html\.generator)
     (package . net\.aserve)
     (package . net\.aserve\.examples)
     (Package . CLIM-INTERNALS)
     (Syntax . Common-lisp)
     (Package . XLIB)
     (Base . 10)
     (Lowercase . Yes)
     (package . store-edit)
     (Syntax . ANSI-Common-Lisp)
     (Package . CL-USER)
     (Package . CALENDAR)
     (syntax . COMMON-LISP)
     (Package . EMACS-CALENDAR)))
 '(save-place-mode t)
 '(show-paren-mode t)
 '(tool-bar-mode nil)
 '(verilog-align-ifelse t)
 '(verilog-auto-delete-trailing-whitespace t)
 '(verilog-auto-inst-param-value t)
 '(verilog-auto-inst-vector nil)
 '(verilog-auto-lineup 'all)
 '(verilog-auto-newline nil)
 '(verilog-auto-save-policy nil)
 '(verilog-auto-template-warn-unused t)
 '(verilog-case-indent 3)
 '(verilog-cexp-indent 3)
 '(verilog-highlight-grouping-keywords t)
 '(verilog-highlight-modules t)
 '(verilog-indent-level 3)
 '(verilog-indent-level-behavioral 3)
 '(verilog-indent-level-declaration 3)
 '(verilog-indent-level-module 3)
 '(verilog-tab-to-comment nil))

;(add-hook 'c-mode-hook #'lsp)
;(add-hook 'c++-mode-hook #'lsp)
