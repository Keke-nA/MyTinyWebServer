#include <iostream>

class A {
  private:
    int a;

  public:
    virtual void f() {};
    A(int a = 0) : a(a) {
    }
};

class B : public A {
  private:
    int b;

  public:
    void f() override {};
};

int main() {
    A* a = new A();
    B* b = new B();

    std::cout << *a << std::endl;
    std::cout << *(void**)b << std::endl;
    return 0;
}