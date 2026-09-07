program test_auto_bounds
   implicit none
   integer :: start_val, end_val, shift

   start_val = 5
   end_val = 10
   shift = 2


   call evaluate_auto_array(start_val)
   call evaluate_complex_auto_array(start_val, end_val, shift)

contains
   subroutine evaluate_auto_array(rows)
      integer, intent(inout) :: rows
      integer :: i

      integer :: arr(rows)

      do i = 1, 5
        arr(i) = i * 10
      end do

      print *, "End" ! Break here 1
   end subroutine evaluate_auto_array

   subroutine evaluate_complex_auto_array(lb, ub, modifier)
      integer, intent(inout) :: lb, ub, modifier
      integer :: i,j,k
      ! Dim 1: (5-2) to (10+2)      -> bounds: 3 to 12
      ! Dim 2: -2 to +2             -> bounds: -2 to 2
      ! Dim 3: 1 to (10-5)       -> bounds: 1 to 5
      integer :: arr(lb - modifier : ub + modifier, &
         0 : modifier * 2, &
         1 : (ub - lb))

      do i = 3, 12
         do j = 0, 4
            do k = 1, 5
               arr(i,j,k) = i * 100 + j * 10 + k
            end do
         end do
      end do

      print *, "End" ! Break here 2
   end subroutine evaluate_complex_auto_array

end program test_auto_bounds
