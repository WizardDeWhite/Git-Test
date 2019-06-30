mod front_of_house {
    pub mod hosting {
        pub fn add_to_waitlist() { }
        fn seat_at_table() { }
    }
    mod serving {
        fn take_over() {}
        fn serve_order() {}
        fn take_payment() {}
    }
}

use crate::front_of_house::hosting;

pub fn eat_at_restaurant() {
    hosting::add_to_waitlist();
}
