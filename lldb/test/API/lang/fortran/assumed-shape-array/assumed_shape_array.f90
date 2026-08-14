program test_assumed_shape
    implicit none

    integer :: arr_1d(-2:2)
    integer :: arr_2d(4, 4)
    integer :: i, j

    ! 1. Initialize 1D array (-2 to 2) -> [10, 20, 30, 40, 50]
    do i = -2, 2
        arr_1d(i) = (i + 3) * 10
    end do

    do j = 1, 4
        do i = 1, 4
            arr_2d(i, j) = i * 10 + j
        end do
    end do

    call process_assumed_1d(arr_1d)

    call process_custom_lb_1d(arr_1d)

    call process_assumed_2d_slice(arr_2d(1:4:2, 1:4:2))

contains

    ! The caller passed (-2:2), but here it resets to (1:5)
    subroutine process_assumed_1d(arr)
        integer, intent(in) :: arr(:)

        print *, arr(1) ! Break here 1
    end subroutine process_assumed_1d

    ! The caller passed (-2:2), but the dummy argument forces lower bound 0 (0:4)
    subroutine process_custom_lb_1d(arr)
        integer, intent(in) :: arr(0:)

        print *, arr(0) ! Break here 2
    end subroutine process_custom_lb_1d

    ! Sliced 2D array: (1:3 step 2, 1:3 step 2) becomes a 2x2 assumed shape
    subroutine process_assumed_2d_slice(arr)
        integer, intent(in) :: arr(:, :)

        print *, arr(1, 1) ! Break here 3
    end subroutine process_assumed_2d_slice

end program test_assumed_shape