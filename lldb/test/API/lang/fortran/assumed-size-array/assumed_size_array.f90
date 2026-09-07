program test_multidim_star
    implicit none
    
    integer :: my_matrix(3, 4)
    integer :: my_simple_matrix(4)
    integer :: r, c

    do c = 1, 4
        my_simple_matrix(c) = c * 10
        do r = 1, 3
            my_matrix(r, c) = r * 10 + c
        end do
    end do

    call process_1d_star_array(my_simple_matrix)

    call process_2d_star_array(my_matrix)

    call process_2d_custom_bounds_star_array(my_matrix)

contains
    subroutine process_1d_star_array(arr)
      integer, intent(in) :: arr(*) 

      print *, arr(1) ! Break here 1
        
    end subroutine process_1d_star_array

    subroutine process_2d_star_array(arr)
      integer, intent(in) :: arr(3, *) 

      print *, arr(1,1) ! Break here 2
        
    end subroutine process_2d_star_array

    subroutine process_2d_custom_bounds_star_array(arr)
      integer, intent(in) :: arr(-2:1, *) 

      print *, arr(-1,1) ! Break here 3
        
    end subroutine process_2d_custom_bounds_star_array

end program test_multidim_star