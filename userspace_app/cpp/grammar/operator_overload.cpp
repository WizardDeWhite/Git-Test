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

int main()
{
	Distance D(5), nD;
	D.displayDistance();
	nD = -D;
	nD.displayDistance();

	return 0;
}
