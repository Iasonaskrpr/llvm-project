program explicit_arrays
    implicit none

    integer :: i, j, k, l, m, n, o
    integer :: val_count

    integer(4) :: arr_1d_default(10)
    real(4) :: arr_1d_custom(-5:5)
    logical :: arr_2d_default(3, 3)
    real(8) :: arr_3d_mixed(0:2, -3:-1, 4:5)
    integer(1) :: arr_7d(1:2, 1:2, 1:2, 1:2, 1:2, 1:2, 1:2)

    do i = 1, 10
        arr_1d_default(i) = i
    end do
    
    do i = -5, 5
        arr_1d_custom(i) = real(i) + 0.5
    end do
    
    val_count = 1
    do j = 1, 3
        do i = 1, 3
            arr_2d_default(i, j) = (mod(val_count, 2) == 0)
            val_count = val_count + 1
        end do
    end do
    
    val_count = 1
    do k = 4, 5
        do j = -3, -1
            do i = 0, 2
                arr_3d_mixed(i, j, k) = real(val_count, 8) + 0.1d0
                val_count = val_count + 1
            end do
        end do
    end do
    
    val_count = 0
    do o = 1, 2
        do n = 1, 2
            do m = 1, 2
                do l = 1, 2
                    do k = 1, 2
                        do j = 1, 2
                            do i = 1, 2
                                arr_7d(i, j, k, l, m, n, o) = int(val_count, 1)
                                val_count = val_count + 1
                            end do
                        end do
                    end do
                end do
            end do
        end do
    end do

    arr_1d_default(1) = 100
    arr_1d_default(10) = 999
    
    arr_1d_custom(0) = 0.0
    arr_1d_custom(-5) = -5.5
    
    arr_2d_default(1, 1) = .true.
    arr_2d_default(3, 3) = .false.
    
    arr_3d_mixed(0, -3, 4) = 3.14159d0
    arr_3d_mixed(2, -1, 5) = 2.71828d0
    
    arr_7d(1,1,1,1,1,1,1) = 1
    arr_7d(2,2,2,2,2,2,2) = 127

    print *, "End" ! Break here

end program explicit_arrays