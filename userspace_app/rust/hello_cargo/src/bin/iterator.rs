fn main() {
    let v1 = vec![1, 2, 3];

    let v1_iter = v1.iter();
    
    for val in v1_iter {
        println!("Got: {}", val);
    }

    // use .next method
    let mut iter = v1.iter();
    loop {
        match iter.next() {
            Some(num) => println!("Has: {}", num),
            None => break,
        }
    }

    // sum()
    let iter2 = v1.iter();

    let total: i32 = iter2.sum();
    println!("Total sum: {}", total);

    // map()
    let v2: Vec<_> = v1.iter().map(|x| x + 1).collect();
    println!("Vec: {:?}", v2);

    // filter()
    filters_by_size();

    // own iterator
    test_own_iterator();

    test_iter_find();
    test_map_fold();
}

#[derive(PartialEq, Debug)]
struct Shoe {
    size: u32,
    style: String,
}

fn shoes_in_my_size(shoes: Vec<Shoe>, shoe_size: u32) -> Vec<Shoe> {
    shoes.into_iter()
        .filter(|s| s.size == shoe_size)
        .collect()
}

fn filters_by_size() {
    let shoes = vec![
        Shoe { size: 10, style: String::from("sneaker") },
        Shoe { size: 13, style: String::from("sandal") },
        Shoe { size: 10, style: String::from("boot") },
    ];

    let in_my_size = shoes_in_my_size(shoes, 10);
    println!("My size shoes: {:?}", in_my_size);
}

// own iterator
#[derive(Debug)]
struct Counter {
    count: u32
}

impl Counter {
    fn new() -> Counter {
        Counter { count: 0}
    }
}

impl Iterator for Counter {
    type Item = u32;

    fn next(&mut self) -> Option<Self::Item> {
        self.count += 1;

        if self.count < 6 {
            Some(self.count)
        } else {
            None
        }
    }
}

fn test_own_iterator() {
    let mut counter = Counter::new();

    loop {
        match counter.next() {
            Some(num) => println!("Counter: {}", num),
            None => break,
        }
    }

    println!("Counter is {:?}", counter);

    let counter2 = Counter::new();

    let val = counter2.skip(1);
    println!("Counter skip 1 {:?}", val);

    let pair = Counter::new().zip(Counter::new().skip(1));

    for e in pair {
        println!("{:?}", e);
    }

    let mul = Counter::new().zip(Counter::new().skip(1)).map(|(a, b)| a * b);
    for e in mul {
        println!("{:?}", e);
    }
}

fn test_iter_find() {
    let list = vec![1, 4, 2, 7, 9];

    match list.iter().find(|&&i| i == 4) {
        None => println!("Not find 4"),
        Some(_) => println!("We find it!"),
    }
}

fn test_map_fold() {
    let vec = vec![1, 2, 4];
    // each element count 1, and do the sum
    println!("vec has {} elements", 
        vec.iter().map(|_| 1).fold(0, std::ops::Add::add));
    // get value of each element, and do the sum
    println!("total number of vec {} ", 
        vec.iter().map(|v| v).fold(0, std::ops::Add::add));
}
