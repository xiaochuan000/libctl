; libctl: flexible Guile-based control files for scientific software 
; Copyright (C) 1998, 1999, 2000, 2001, Steven G. Johnson
;
; This library is free software; you can redistribute it and/or
; modify it under the terms of the GNU Lesser General Public
; License as published by the Free Software Foundation; either
; version 2 of the License, or (at your option) any later version.
;
; This library is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
; Lesser General Public License for more details.
; 
; You should have received a copy of the GNU Lesser General Public
; License along with this library; if not, write to the
; Free Software Foundation, Inc., 59 Temple Place - Suite 330,
; Boston, MA  02111-1307, USA.
;
; Steven G. Johnson can be contacted at stevenj@alum.mit.edu.

; ****************************************************************

(if (not (defined? 'material-type))
    (define-class material-type no-parent)) ; define dummy class

(define-class geometric-object no-parent
  (define-property material 'material-type no-default)
  (define-property center 'vector3 no-default))

(define (non-negative? x) (not (negative? x)))

(define-class cylinder geometric-object
  (define-post-processed-property axis 'vector3 unit-vector3
    (make-default (vector3 0 0 1)))
  (define-property radius 'number no-default non-negative?)
  (define-property height 'number no-default non-negative?))

(define-class cone cylinder
  (define-property radius2 'number (make-default 0)))
	       
(define-class sphere geometric-object
  (define-property radius 'number no-default non-negative?))
	       
(define-class block geometric-object
  (define-post-processed-property e1 'vector3 unit-vector3 
    (make-default (vector3 1 0 0)))
  (define-post-processed-property e2 'vector3 unit-vector3 
    (make-default (vector3 0 1 0)))
  (define-post-processed-property e3 'vector3 unit-vector3 
    (make-default (vector3 0 0 1)))
  (define-property size 'vector3 no-default)
  (define-derived-property projection-matrix 'matrix3x3
    (lambda (object)
      (matrix3x3-inverse
       (matrix3x3
	(object-property-value object 'e1)
	(object-property-value object 'e2)
	(object-property-value object 'e3))))))

(define-class ellipsoid block
  (define-derived-property inverse-semi-axes 'vector3
    (lambda (object)
      (vector-map (lambda (x) (/ 2.0 x))
		  (object-property-value object 'size)))))

; ****************************************************************

(define-class lattice no-parent
  (define-post-processed-property basis1 'vector3 unit-vector3 
    (make-default (vector3 1 0 0)))
  (define-post-processed-property basis2 'vector3 unit-vector3 
    (make-default (vector3 0 1 0)))
  (define-post-processed-property basis3 'vector3 unit-vector3 
    (make-default (vector3 0 0 1)))

  (define-property size 'vector3 (make-default (vector3 1 1 1)))
  (define-property basis-size 'vector3 (make-default (vector3 1 1 1)))

  (define-derived-property b1 'vector3
    (lambda (object)
      (vector3-scale (vector3-x (object-property-value object 'basis-size))
				(object-property-value object 'basis1))))
  (define-derived-property b2 'vector3
    (lambda (object)
      (vector3-scale (vector3-y (object-property-value object 'basis-size))
				(object-property-value object 'basis2))))
  (define-derived-property b3 'vector3
    (lambda (object)
      (vector3-scale (vector3-z (object-property-value object 'basis-size))
				(object-property-value object 'basis3))))

  (define-derived-property basis 'matrix3x3
    (lambda (object)
      (let ((B (matrix3x3
		(object-property-value object 'b1)
		(object-property-value object 'b2)
		(object-property-value object 'b3))))
	(if (zero? (matrix3x3-determinant B))
	    (error "lattice basis vectors must be linearly independent!"))
	B)))
  (define-derived-property metric 'matrix3x3
    (lambda (object)
      (let ((B (object-property-value object 'basis)))
	(matrix3x3* (matrix3x3-transpose B) B)))))

; ****************************************************************

; Define some utility functions:

(define (shift-geometric-object go shift-vector)
  (let ((c (object-property-value go 'center)))
    (modify-object go (center (vector3+ c shift-vector)))))

(define (geometric-object-duplicates shift-vector min-multiple max-multiple go)
  (define (g-o-d min-multiple L)
    (if (<= min-multiple max-multiple)
	(g-o-d (+ min-multiple 1)
	       (cons (shift-geometric-object
		      go (vector3-scale min-multiple shift-vector))
		     L))
	L))
  (g-o-d min-multiple '()))

(define (geometric-objects-duplicates shift-vector min-multiple max-multiple
				      go-list)
  (fold-left append '()
	     (map (lambda (go)
		    (geometric-object-duplicates
		     shift-vector min-multiple max-multiple go))
		  go-list)))

(define (geometric-objects-lattice-duplicates go-list . usize)
  (let ((u1 (if (>= (length usize) 1) (list-ref usize 0) 1))
	(u2 (if (>= (length usize) 2) (list-ref usize 1) 1))
	(u3 (if (>= (length usize) 3) (list-ref usize 2) 1)))
    (let ((b1 (vector3 u1 0 0)) (b2 (vector3 0 u2 0)) (b3 (vector3 0 0 u3))
	  (n1 (/ (vector3-x (object-property-value geometry-lattice 'size))
		 u1))
	  (n2 (/ (vector3-y (object-property-value geometry-lattice 'size))
		 u2))
	  (n3 (/ (vector3-z (object-property-value geometry-lattice 'size))
		 u3)))
      (geometric-objects-duplicates
       b1 (- (floor (/ (- n1 1) 2))) (ceiling (/ (- n1 1) 2))
       (geometric-objects-duplicates
	b2 (- (floor (/ (- n2 1) 2))) (ceiling (/ (- n2 1) 2))
	(geometric-objects-duplicates
	 b3 (- (floor (/ (- n3 1) 2))) (ceiling (/ (- n3 1) 2))
	 go-list))))))

; ****************************************************************

(define-input-var dimensions 3 'integer)
(define-input-var default-material '() 'material-type)
(define-input-var geometry-lattice (make lattice) 'lattice)
(define-input-var geometry '() (make-list-type 'geometric-object))
(define-input-var ensure-periodicity true 'boolean)

(define-external-function point-in-object? true false
  'boolean 'vector3 'geometric-object)

(define-external-function point-in-periodic-object? true false
  'boolean 'vector3 'geometric-object)

; (define-external-function material-of-point true false
;   'material-type 'vector3)

(define-external-function display-geometric-object-info false false
  no-return-value 'integer 'geometric-object)

(define-external-function square-basis false false
  'matrix3x3 'matrix3x3 'vector3)

; ****************************************************************
; Functions and variables for determining the grid size

(define no-size 1e-20) ; for when a particular lattice dimension has no size
(define-param resolution 10)   ; the resolution (may be a vector3)
(define-param grid-size false) ; force grid size, if set

(define (get-grid-size)
  (if grid-size
      grid-size
      (let ((res (if (vector3? resolution)
		     resolution
		     (vector3 resolution resolution resolution))))
	(vector-map
	 (lambda (x) (inexact->exact (max (ceiling x) 1)))
	 (vector-map * res (object-property-value geometry-lattice 'size))))))

; ****************************************************************
; Cartesian conversion and rotation for lattice and reciprocal coords:

; The following conversion routines work for vector3 and matrix3x3 arguments:

(define (lattice->cartesian x)
  (if (vector3? x)
      (matrix3x3*
       (object-property-value geometry-lattice 'basis) x)
      (matrix3x3* 
       (matrix3x3* 
	(object-property-value geometry-lattice 'basis) x)
       (matrix3x3-inverse (object-property-value geometry-lattice 'basis)))))

(define (cartesian->lattice x)
  (if (vector3? x)
      (matrix3x3*
       (matrix3x3-inverse (object-property-value geometry-lattice 'basis)) x)
      (matrix3x3* 
       (matrix3x3*
	(matrix3x3-inverse (object-property-value geometry-lattice 'basis)) x)
       (object-property-value geometry-lattice 'basis))))

(define (reciprocal->cartesian x)
  (let ((s (object-property-value geometry-lattice 'size)))
    (let ((Rst
	   (matrix3x3-transpose
	    (matrix3x3* (object-property-value geometry-lattice 'basis)
			(matrix3x3 (vector3 (vector3-x s) 0 0)
				   (vector3 0 (vector3-y s) 0)
				   (vector3 0 0 (vector3-z s)))))))
      (if (vector3? x)
	  (matrix3x3* (matrix3x3-inverse Rst) x)
	  (matrix3x3* (matrix3x3* (matrix3x3-inverse Rst) x) Rst)))))

(define (cartesian->reciprocal x)
  (let ((s (object-property-value geometry-lattice 'size)))
    (let ((Rst
	   (matrix3x3-transpose
	    (matrix3x3* (object-property-value geometry-lattice 'basis)
			(matrix3x3 (vector3 (vector3-x s) 0 0)
				   (vector3 0 (vector3-y s) 0)
				   (vector3 0 0 (vector3-z s)))))))
      (if (vector3? x)
	  (matrix3x3* Rst x)
	  (matrix3x3* (matrix3x3* Rst x) (matrix3x3-inverse Rst))))))

(define (lattice->reciprocal x) (cartesian->reciprocal (lattice->cartesian x)))
(define (reciprocal->lattice x) (cartesian->lattice (reciprocal->cartesian x)))

; rotate vectors in lattice/reciprocal coords (note that the axis
; is also given in the corresponding basis):

(define (rotate-lattice-vector3 axis theta v)
  (cartesian->lattice
   (rotate-vector3 (lattice->cartesian axis) theta
		   (lattice->cartesian v))))

(define (rotate-reciprocal-vector3 axis theta v)
  (cartesian->reciprocal
   (rotate-vector3 (reciprocal->cartesian axis) theta
		   (reciprocal->cartesian v))))

; ****************************************************************

