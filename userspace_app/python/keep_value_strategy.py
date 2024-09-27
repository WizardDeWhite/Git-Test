#!/usr/bin/python
import argparse

parser = argparse.ArgumentParser(description='do keey value strategy')
parser.add_argument("-t", "--target_price", type=float, default=1.5,
                  help="target price")
parser.add_argument("-i", "--increment", type=float, default=0.1,
                  help="increment step")
args = parser.parse_args()

increment = args.increment
target_price = args.target_price
if __name__ == "__main__":
    initial_price = 1.0
    shares = 100
    initial_value = initial_price * shares
    threshold = 100
    total_value = 0.0

    
    current_price = initial_price * (1 + increment)
    while current_price < target_price:
        current_value = shares * current_price

        print('+{:.2%}'.format(increment))
        print("\tcurrent shares %d" % shares)
        print("\tcurrent price %0.3f" % current_price)
        print("\tcurrent value %0.3f" % current_value)

        if current_value > threshold:
            sell_value = current_value - threshold
            sell_shares = int(sell_value / current_price)
            sell_value = sell_shares * current_price
            shares -= sell_shares
            current_value = shares * current_price
            total_value += sell_value

            print("\tsell shares %d" % sell_shares)
            print("\tsell value %0.3f" % sell_value)
        else:
            print("not enough for sale")

        current_price *= (1 + increment)
    
    current_price = target_price
    current_value = shares * current_price
    print("%0.2f" % target_price)
    print("\tlast shares %d" % shares)
    print("\tlast price %0.3f" % current_price)
    print("\tlast value %0.3f" % current_value)

    total_value += current_value
    print("total value %0.3f" % total_value)
    print("total profit %0.3f" % (total_value - initial_value ))
