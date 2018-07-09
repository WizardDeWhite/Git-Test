#include <iostream>

using namespace std;

class Distance
{
   private:
      int inches;           // 0 �� 12
   public:
      // ����Ĺ��캯��
      Distance(){
         inches = 0;
      }
      Distance(int i){
         inches = i;
      }
      // ��ʾ����ķ���
      void displayDistance()
      {
         cout << " I:" << inches <<endl;
      }
      // ���ظ�������� - ��
      Distance operator- ()  
      {
         inches = -inches;
         return Distance(inches);
      }
};

class Point
{
public:

      int x, y;
      Point(int _x, int _y):x(_x),y(_y){}
      operator Distance()const {return Distance(y-x);}
};

int main()
{
	Distance D(5), nD;
	D.displayDistance();
	nD = -D;
	nD.displayDistance();

	Point P(2, 8);
	nD = P;

	return 0;
}
