//	TProgressDialog is opened to notify the user of the progress of
//	the folding algorithm

class TProgressDialog 
{
	public:

   	void update(int frac) {
		cout << frac <<"%\n";
	}
};
