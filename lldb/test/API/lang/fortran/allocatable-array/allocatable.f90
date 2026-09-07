program allocatables
    implicit none
  
    integer, allocatable :: arr_unallocated(:,:)
    
    integer, allocatable :: arr_1d_zero_size(:)
    
    real(4), allocatable :: arr_1d_neg_bounds(:)
    
    logical, allocatable :: arr_2d_weird_bounds(:,:)
    
    real(8), allocatable :: arr_3d_dynamic(:,:,:)
    
    integer(1), allocatable :: arr_7d_cursed(:,:,:,:,:,:,:)

    integer, target, allocatable :: arr_base(:)
    integer, pointer :: arr_stride_2(:)
    integer, pointer :: arr_stride_reverse(:)

    integer :: dyn_lower, dyn_upper
    integer :: i, j, k, l, m, n, o
    integer :: val_count


    allocate(arr_1d_zero_size(10:5)) 


    allocate(arr_1d_neg_bounds(-10:-5))
    do i = -10, -5
        arr_1d_neg_bounds(i) = real(i) * 1.5
    end do
    arr_1d_neg_bounds(-10) = -999.9  
    arr_1d_neg_bounds(-5) = 555.5


    allocate(arr_2d_weird_bounds(-2:2, 0:0))
    do j = 0, 0
        do i = -2, 2
            arr_2d_weird_bounds(i, j) = (mod(abs(i), 2) == 1)
        end do
    end do
    arr_2d_weird_bounds(0, 0) = .true. 

    dyn_lower = -3
    dyn_upper = 2
    allocate(arr_3d_dynamic(dyn_lower:dyn_upper, 5:7, -1:1))
    
    val_count = 1
    do k = -1, 1
        do j = 5, 7
            do i = dyn_lower, dyn_upper
                arr_3d_dynamic(i, j, k) = real(val_count, 8) + 0.5d0
                val_count = val_count + 1
            end do
        end do
    end do
    arr_3d_dynamic(0, 6, 0) = 2.71828d0 

    allocate(arr_7d_cursed(-1:0, 1:2, -2:-1, 0:1, -3:-2, 2:3, -4:-3))
    
    val_count = 0
    do o = -4, -3
        do n = 2, 3
            do m = -3, -2
                do l = 0, 1
                    do k = -2, -1
                        do j = 1, 2
                            do i = -1, 0
                                arr_7d_cursed(i, j, k, l, m, n, o) = int(val_count, 1)
                                val_count = val_count + 1
                            end do
                        end do
                    end do
                end do
            end do
        end do
    end do

    arr_7d_cursed(-1, 1, -2, 0, -3, 2, -4) = 42
    arr_7d_cursed(0, 2, -1, 1, -2, 3, -3) = -128

    allocate(arr_base(10))
    do i = 1, 10
        arr_base(i) = i * 10
    end do

    arr_stride_2 => arr_base(1:10:2)

    arr_stride_reverse => arr_base(10:1:-1)

    print *, "End" ! Break here

end program allocatables