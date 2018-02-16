
#|
Benchmarks & tests
|#
;summation
(define (sum x)
  (letrec ([do (lambda (x sum) 
                 (cond ((= x 0) sum)
                       (else (do (- x 1) (+ sum x)))))])
    (do x 0)))



(define (fact n)
  (if (= n 0)
      1
      (* n (fact (- n 1)))))

(define nil ())

(define append list-cat)

(append 'a 'b `(5 ,nil ,@(list (fact 7) (fact 8)) 5))
; list reverse
(define (lrev L)
  (if (eq? (cdr L) nil)
      L
      (append (lrev (cdr L))
              (list (car L)))))

(define (sqr x) (* x x))
(define (f*2 f1 f2 x) (f1 (f2 x)))

; generates list
(define (lgen gen)
  (letrec [(loop (lambda (l)
                   (let [(g (gen))]
                     (if (eq? g 'end)
                         l
                         (loop (cons g l))))))]
    (lrev (loop ()))))

(define (genseq from to step)
  (let [(cur (- from step))]
    (lambda () 
      (set! cur (+ cur step))
      (if (or (< cur to) (= cur to)) 
          cur
          'end))))

; create lambda which is generator of vector elements
(define (genvec v)
  (let [(cur -1) (to (vector-length v))]
    (lambda () 
      (set! cur (+ cur 1))
      (if (< cur to) 
          (vector-ref v cur)
          'end))))

(define (gencrand to)
  (let [(cur -1)]
    (lambda () 
      (set! cur (+ cur 1))
      (if (< cur to) 
          (crand)
          'end))))


(define (vector->list vec) (lgen (genvec vec))) 

(define (map fun _src)
  (letrec [(loop 
            (lambda (dest src)
              (if (eq? () src)
                  dest
                  (loop (cons (fun (car src)) dest) (cdr src)))))]
    (lrev (loop () _src))))
 
; test list generation
(define (test c)
  (map  (lambda (x) (lgen (genseq 1 x 1)))  (lgen (genseq 1 c 1)) ))

; measure dur
(define (dur fun)
  (let [(t (time))
        (v (fun))]
    (list (cons 'value= v) 
          (cons 'time= (-(time) t) ))))

(define (dur1 fun arg) 
  (dur (lambda () (fun arg))))

#;(map  (lambda (x) (lgen (genseq 1 x 1)))  '(1 2 3 4 5)  )  

(define n  (lgen (genseq 1 1 1)))

(define test-qslr (lambda (x) (list-qsort (lrev (lgen (genseq 1 x 1))))))
#;(cond (#t 12))



#;(define (>10 x) (if (number? x) (> x 10) #f))

(define (list-qsort L)
  (if (null? L)
      ()
      (append
       (list-qsort (list< (car L) (cdr L)) )
       (car L)
       (list-qsort (list>= (car L) (cdr L)) ))))


(define (list-filter  filter L)
  ; Makes a copy of list L
  ; containing only elements
  ; that are < N.
  ; L must be a list of numbers.
  (if (null? L)
      ()
      (if (filter (car L))
          (cons (car L)
                (list-filter filter (cdr L)))
          (list-filter filter (cdr L)))))

(define (list< x l)  
  (list-filter (lambda (e) (< e x)) l) )

(define (list>= x l)  
  (list-filter (lambda (e) (>= e x)) l) )

(define (lgen-rand x) (lgen (gencrand x)))

(lrev (lgen (genvec #(a b c d))))

(assert (equal? (lrev '(a b c)) '(c b a))
        "lrev failed")

(define (test-gen count)
  (lgen (genseq 1 count 1)))

(define (test-qsort count)
  (list-qsort (lrev (test-gen count))))

(define (test-gc count)
  (letrec ([iter 300]
           [loop
            (lambda (i)
              (if (not (= i 0))
                  (let ([r (dur1 test iter)])
                    (display "Test-gc time: "
                             (car (cdr r))
                             "\n")
                    (assert (=  (length  (cdr (car r))) iter) 
                            "Test-gc Failed")
                    (loop (- i 1)))))])
    (loop count)))


(define (fac x)
  (if (= x 0) 
      1
      (* x (fac (- x 1)))))

(define (power x i)
  (if (= i 0)
      1
      (* x (power x (- i 1)))))
      

(define (sine x count)
  (letrec ([max (* 2 count)] 
           [next
            (lambda (sgn p)
              (if (< p count)
                  (+ (* sgn (/ (power x p) (fac p) ))
                     (next (* -1 sgn) (+ p 2)))
                  0))])
    (next 1 1.0)))
                     
(define (cosine x count)
  (letrec ([max (* 2 count)] 
           [next
            (lambda (sgn p)
              (if (< p count)
                  (+ (* sgn (/ (power x p) (fac p) ))
                     (next (* -1 sgn) (+ p 2)))
                  0))])
    (next 1 0.0)))

(define (test-num count)
  (letrec ([angle 1.0]
           [iter  100.0]
           [sum   0]
           [loop
            (lambda (i)
              (cond ((= i 0) sum)
                    (else
                     (set! sum (+ sum 
                                  (+ (power (sine angle iter) 2)
                                     (power (cosine angle iter) 2))))
                     (loop (- i 1)))))])
    (loop count)))
                     

;some tests
(let* [(l (lrev '(1 2 3 4 5)))]
  (assert (equal? (list-qsort l) '(1 2 3 4 5))
          "list-qsort failed")
  (assert (equal? (sum 5) (+ 1 2 3 4 5))
          "sum failed")
  (assert (equal? (map 
                  (lambda(x)(* x 2))
                 '(1 2 3))
                 '(2 4 6))
          "map failed")
  (display "Tests - ok.\n"))
  

#;(if (if (if (if x t) x) y) z z1)
;(if (if x (lambda (x) x) x) 12 3)
#|
(if (let (x 8) x) 2)
(((nested)) 12)

(+ 3 (begin (begin 1 (set! y 12) (let ((t y)) (car x t)) 2)))

(+ 1 (begin  (let ((x 1) (z 2)) 
                (if (eq? (car t) 'empty)
    (cadr t)
    (+ (caddr t) (- (check (cadr t)) (check (cadddr t)))))
              (letrec ([y x])
                (set! y 12)
                 (cadr t)
    (+ (caddr t) (- (check (cadr t)) (check (cadddr t)))))
              (letrec ([y x])
                (set! y 12)
                (dris)
                (if #f (* x y)) ))))
           
#;(+ 3 (begin (begin 1 (car 3 4)) 2))
#;(+ (- (if (car '(#t #f) ) (begin #\a #\b) ) (* 4 5))  (/ (* x (car) (cdr))) )
(define main (lambda (argv)
  (let* ((min-depth 4)
         (max-depth (max (+ min-depth 2) (if (pair? argv) (string->number (car argv)) 10))))
    (let ((stretch-depth (+ max-depth 1)))
      (display "stretch tree of depth ") (display stretch-depth) (write-char #\tab) (display " check: ") (display (check (make 0 stretch-depth))) (newline))
    (let ((long-lived-tree (make 0 max-depth)))
      (do ((d 4 (+ d 2))
           (c 0 0))
        ((> d max-depth))
        (let ((iterations (arithmetic-shift 1 (+ (- max-depth d) min-depth)))) ; chicken-specific: arithmetic-shift
          (do ((i 0 (+ i 1)))
            ((>= i iterations))
            (set! c (+ c (check (make i d)) (check (make (- i) d)))))
          (display (* 2 iterations)) (write-char #\tab) (display " trees of depth ") (display d) (write-char #\tab) (display " check: ") (display c) (newline)))
      (display "long lived tree of depth ") (display max-depth) (write-char #\tab) (display " check: ") (display (check long-lived-tree)) (newline)))))




#|
(cond (x y z) (w v) (else a b))

(cond (else x y))
(define let 12)


(let ((x u))
(define (let ([x 1] [x 2])
  (cond (else 1))
  (set! x '(1 2 3))
  (cond (x y z) (w v) (else a b)) 
  (list 1)
  (lambda (x y  z) x ))  (x x y . y) x) )


(cond (else x y))

(cond ((cond (else (cond (x y)))) z)  (else (cond (x y))))

(lambda (x y z r y))
(let ([x 1] [x 2])
  (cond (else 1))
  (set! x '(1 2 3))
  (cond (x y z) (w v) (else a b)) 
  (list 1)
  (lambda (x y  z) x )) 

(define (proc arg1 arg2) 12 13 14 '()) |#


((lambda (x) x) 12)
#;(let ([x 1] [x 2]) x y)

#;(define make (lambda (item d) 
  (if (= d 0)
    (list 'empty item)
    (let () ((item2 (* item 2))
          (d2 (- d 1)))
      (list 'node (make (- item2 1) d2) item (make item2 d2))))))


#|
(define check (lambda (t)
  (if (eq? (car t) 'empty)
    (cadr t)
    (+ (caddr t) (- (check (cadr t)) (check (cadddr t)))))))

(define main (lambda (argv)
  (let* ((min-depth 4)
         (max-depth (max (+ min-depth 2) (if (pair? argv) (string->number (car argv)) 10))))
    (let ((stretch-depth (+ max-depth 1)))
      (display "stretch tree of depth ") (display stretch-depth) (write-char #\tab) (display " check: ") (display (check (make 0 stretch-depth))) (newline))
    (let ((long-lived-tree (make 0 max-depth)))
      (do ((d 4 (+ d 2))
           (c 0 0))
        ((> d max-depth))
        (let ((iterations (arithmetic-shift 1 (+ (- max-depth d) min-depth)))) ; chicken-specific: arithmetic-shift
          (do ((i 0 (+ i 1)))
            ((>= i iterations))
            (set! c (+ c (check (make i d)) (check (make (- i) d)))))
          (display (* 2 iterations)) (write-char #\tab) (display " trees of depth ") (display d) (write-char #\tab) (display " check: ") (display c) (newline)))
      (display "long lived tree of depth ") (display max-depth) (write-char #\tab) (display " check: ") (display (check long-lived-tree)) (newline)))))
|# |#
