;; -*- lexical-binding: t -*-
;; Functions for applying overlays to Faint source files in GNU Emacs.
;; Usage:
;;   1. Specify the path to the definition file (faint-defs-file-name)
;;   2. Call faint-generate-defs
;;   3. Call faint-load-defs
;;   4. Use faint-start-magic in a Faint source code buffer.

(load-library "faint-paths.el")

(defvar faint-defs-list ()
  "List containing the definitions parsed by `faint-load-defs' from the file specified by `faint-defs-file-name'")


(defun slurp (f)
  (with-temp-buffer
    (insert-file-contents f)
    (buffer-substring-no-properties
     (point-min)
     (point-max))))


(defun faint-generate-defs ()
  (interactive)
  (compile (concat (faint-code-utils-dir) "find_defs.py")))


(defun faint-load-defs ()
  "Load the text file of references and defines.
The path to the defines is specified by `faint-defs-file-name'.
The file is generated by \"find_defs.py\"."
  (interactive)
  (let ((res ()))
    (dolist (x (split-string
                (slurp (faint-defs-file-name)) "===" t))
      (let ((p (split-string x "---")))
        (push (cons (car p) (cdr p)) res)))
    (setq faint-defs-list res)))


(defun link-to-def-action (button)
  (find-file (button-get button 'target-file))
  (goto-char (button-get button 'target-char)))


(define-button-type 'link-to-def-button
  'follow-link t
  'action #'link-to-def-action)


(defun ref-overlay (start end str f char)
  (let ((ovl (make-overlay start end nil t t)))
    (overlay-put ovl 'face '((:background "#B5A5D5") (:foreground "#000000")))
    (overlay-put ovl 'mouse-face '(:underline t))
    (overlay-put ovl 'evaporate t)
    (overlay-put ovl 'intangible t)
    (overlay-put ovl 'display str))

  ;; Make the reference a link to the definition
  (let ((button (make-button start end :type 'link-to-def-button)))
    (button-put button 'target-file f)
    (button-put button 'target-char char)))


(defun def-overlay (start end str f char)
  (let ((ovl (make-overlay start end nil t t)))
    (overlay-put ovl 'evaporate t)
    (overlay-put ovl 'intangible t)
    (overlay-put ovl 'face '((:height 200) "bold"))
    (overlay-put ovl 'display str)))


(defun license-overlay (start end)
  (let ((ovl (make-overlay start end nil t t)))
    (overlay-put ovl 'intangible t)
    (overlay-put ovl 'display "License preamble")
    (overlay-put ovl 'face "italic")))


(defun def-text (def)
  (cdddr def))


(defun def-file (def)
  (cadr def))


(defun def-char (def)
  (string-to-number (caddr def)))


(defun find-def-for-ref (ref)
  (assoc ref faint-defs-list))


(defun refs-defs ()
  ;; Turns references in the form
  ;;  \ref(<label>)
  ;; into links to the definition, in the form
  ;;  \def(<label>)title;
  ;;
  ;; Requires that definitions have been loaded with faint-load-defs.
  (save-excursion
    (beginning-of-buffer)
    (while (search-forward-regexp "\\\\ref(\\(.*?\\))" nil t)
      (let ((outer (list (match-beginning 0) (match-end 0)))
            (inner (list (match-beginning 1) (match-end 1))))
      (let ((item (find-def-for-ref (apply 'buffer-substring inner))))
        (ref-overlay (car outer) (cadr outer)
                     (def-text item)
                     (def-file item)
                     (def-char item)))))

    (while (search-forward-regexp "\\\\def(\\(.*?\\)).*?;" nil t)
      (let ((outer (list (match-beginning 0) (match-end 0)))
            (inner (list (match-beginning 1) (match-end 1))))
      (let ((item (find-def-for-ref (apply 'buffer-substring inner))))
        (def-overlay (car outer) (cadr outer)
                     (def-text item)
                     (def-file item)
                     (def-char item)))))))



(defun license ()
  (save-excursion
    (beginning-of-buffer)
    (when (search-forward-regexp
           "// -\\*\\(?:.*?\n\\)*.*?under the License." nil t)
      (let ((outer (list (match-beginning 0) (match-end 0))))
        (license-overlay (car outer) (cadr outer))))))


(defun verify-overlay (start end entry)
  (let ((ovl (make-overlay start end nil t t)))
    (overlay-put ovl 'display (concat "> " entry))
    (overlay-put ovl 'face '((:background "#DCDCDC") "bold"))))


(defun verify ()
  ;; Apply silly overlays to the VERIFY-macro in tests
  (save-excursion
    (beginning-of-buffer)
    (while (search-forward-regexp "VERIFY(\\(.*\\));" nil t)
    (let ((outer (list (match-beginning 0) (match-end 0)))
          (entry (list (match-beginning 1) (match-end 1))))
      (verify-overlay (car outer) (cadr outer)
                      (apply 'buffer-substring entry))))))


(defun faint-start-magic ()
  "Apply Faint-specific overlays to the current buffer."
  (interactive)
  (faint-stop-magic) ; Reset current overlays
  (refs-defs) ; Make references into links
  (license) ; Hide the license-preamble
  (verify) ; Nicen the appearance of unit-test VERIFY macros
)


(defun faint-stop-magic ()
  "Remove Faint-specific overlays from the current buffer"
  (interactive)
  (remove-overlays))
