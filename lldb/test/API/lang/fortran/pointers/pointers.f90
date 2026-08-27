program simple_pointers
    implicit none

    ! 1. Abstract interface to declare the function pointer signature
    abstract interface
        integer function square_func(x)
            integer, intent(in) :: x
        end function square_func
    end interface

    integer, target :: target_val = 42
    integer, pointer :: int_ptr => null()

    procedure(square_func), pointer :: func_ptr => null()

    int_ptr => target_val
    print *, "Pointer value:", int_ptr

    func_ptr => square
    print *, "Function call via pointer (4^2):", func_ptr(4) ! Break here

contains

    integer function square(x)
        integer, intent(in) :: x
        square = x * x
    end function square

end program simple_pointers