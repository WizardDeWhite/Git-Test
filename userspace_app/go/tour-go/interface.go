package main

import (
	"fmt"
	"math"
)

type Abser interface {
	Abs() float64
}

type Abber interface {
	Abb() float64
}

type MyFloat float64

func (f MyFloat) Abs() float64 {
	if f < 0 {
		return float64(-f)
	}
	return float64(f)
}

type Vertex struct {
	X, Y float64
}

func (v *Vertex) Abs() float64 {
	return math.Sqrt(v.X*v.X + v.Y*v.Y)
}

func (v *Vertex) Abb() float64 {
	return v.X + 10
}

func generic_type() {
	var a Abser
	var b Abber
	f := MyFloat(-math.Sqrt2)
	v := Vertex{3, 4}

	a = f  // a MyFloat implements Abser
	a = &v // a *Vertex implements Abser

	// In the following line, v is a Vertex (not *Vertex)
	// and does NOT implement Abser.
	//a = v

	fmt.Println(a.Abs())

	b = &v
	fmt.Println(b.Abb())
}

func real_type_of_interface() {
	var i interface{}
	i = "hello"

	s := i.(string)
	fmt.Println(s)

	s, ok := i.(string)
	fmt.Println(s, ok)

	f, ok := i.(float64)
	fmt.Println(f, ok)

	i = 100
	t, ok := i.(int)
	fmt.Println(t, ok)
}

func Print(i interface{}) {
	switch i.(type) {
	case string:
		fmt.Printf("type is string,value is:%v\n", i.(string))
	case float64:
		fmt.Printf("type is float32,value is:%v\n", i.(float64))
	case int:
		fmt.Printf("type is int,value is:%v\n", i.(int))
	}
}

func use_switch_to_match_type() {
	var i interface{}
	i = "hello"
	Print(i)
	i = 100
	Print(i)
	i = 1.29
	Print(i)
}

type Ab interface {
	Abser
	Abber
}

var _ Ab = &NopAb{}

type NopAb struct{}

func (a *NopAb) Abs() float64 {
	return 0.0
}
func (a *NopAb) Abb() float64 {
	return 0.0
}

var _ Ab = &AbImpl{}

type AbImpl struct {
	Val   float64
	NopAb // embed a struct which implements the interface
}

// while the method could be override
func (a *AbImpl) Abs() float64 {
	return a.Val
}

func test_AbImpl() {
	var a AbImpl
	a.Val = 9.8
	fmt.Println(a.Abs())
	fmt.Println(a.Abb())
}

type eIntf struct {
	Val   float64
	Abber // embed an interface
}

func test_embedInterface() Abber {
	var tx eIntf
	tx.Val = 9.0
	tx.Abber = &Vertex{0.0, 0.0} // assign to a real interface struct
	fmt.Println("tx.Abb() ", tx.Abb())

	tx2 := eIntf{
		Val: 9.0,
		// panic if not assigned a real interface struct
		// Abber: &Vertex{0.0, 0.0},
	}
	if tx2.Abber != nil {
		fmt.Println("tx2.Abb() ", tx2.Abb())
	}
	return tx
}

func main() {
	// generic_type()
	// real_type_of_interface()
	// use_switch_to_match_type()
	// test_AbImpl()
	test_embedInterface()
}
