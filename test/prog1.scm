(define (sum x)
  (letrec ([do (lambda (x sum) 
                 (cond ((= x 0) sum)
                       (else (do (- x 1) (+ sum x)))))])
    (do x 0)))

(define nil '())

(define (lrev L)
  (if (eq? (cdr L) nil)
      L
      (append (lrev (cdr L))
              (list (car L)))))

(define (dur fun)
  (let [(t (current-milliseconds))
        (v (fun))]
    (list (cons 'value= v) 
          (cons 'time= (-(current-milliseconds) t) ))))

(define (dur1 fun arg) 
  (dur (lambda () (fun arg))))

(define (test-gen count)
  (lgen (genseq 1 count 1)))

(define (genseq from to step)
  (let [(cur (- from step))]
    (lambda () 
      (set! cur (+ cur step))
      (if (or (< cur to) (= cur to)) 
          cur
          'end))))

(define (lgen gen)
  (letrec [(loop (lambda (l)
                   (let [(g (gen))]
                     (if (eq? g 'end)
                         l
                         (loop (cons g l))))))]
    (lrev (loop ()))))

(define (test-qsort count)
  (list-qsort (lrev (test-gen count))))

(define (list-qsort L)
  (if (null? L)
      ()
      (append
       (list-qsort (list< (car L) (cdr L)) )
       (list (car L))
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
    (next 1 1)))
                     
(define (cosine x count)
  (letrec ([max (* 2 count)] 
           [next
            (lambda (sgn p)
              (if (< p count)
                  (+ (* sgn (/ (power x p) (fac p) ))
                     (next (* -1 sgn) (+ p 2)))
                  0))])
    (next 1 0)))

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


