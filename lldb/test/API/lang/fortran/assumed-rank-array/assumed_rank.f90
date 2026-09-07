program main
    implicit none

    integer :: scalar_val
    integer, dimension(4) :: vec
    integer, dimension(2, 3) :: matrix
    integer, dimension(2, 2, 2) :: tensor

    scalar_val = 42
    vec = [10, 20, 30, 40]
    matrix = reshape([1, 2, 3, 4, 5, 6], shape=[2, 3])
    tensor = 99 
    tensor(2,1,2) = 11

    call analyze_array(scalar_val, vec, matrix, tensor)

contains

    subroutine analyze_array(scalar_val, vec, matrix, tensor)
        integer, intent(in) :: scalar_val(..), vec(..), matrix(..), tensor(..)

        print *, "Detected rank for scalar_val:", rank(scalar_val)
        print *, "Detected rank for vec:", rank(vec) 
        print *, "Detected rank for matrix:", rank(matrix) 
        print *, "Detected rank for tensor:", rank(tensor) ! Break here
        
    end subroutine analyze_array

end program main