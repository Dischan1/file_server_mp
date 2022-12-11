//                                                 //
// https://github.com/kevhou/RSA/blob/master/RSA.c //
//                                                 //

int rsa_genprime()
{
	for (int p = rand() % 64 + 11; p > 0; --p)
	{
		for (int i = 2; i <= sqrt(p); i++)
		{
			if (p % i == 0)
				goto lskip;
		}

		return p;
	lskip:;
	}
	return 11;
}

// Find the Greatest Common Divisor between two numbers
int rsa_gcd(int num1, int num2) {
	int temp;

	while (num2 > 0) {
		temp = num1 % num2;
		num1 = num2;
		num2 = temp;
	}

	return num1;
}

// d = (1/e) mod n
int rsa_mod_inverse(int u, int v)
{
	int inv, u1, u3, v1, v3, t1, t3, q;
	int iter;
	/* Step X1. Initialise */
	u1 = 1;
	u3 = u;
	v1 = 0;
	v3 = v;
	/* Remember odd/even iterations */
	iter = 1;
	/* Step X2. Loop while v3 != 0 */
	while (v3 != 0)
	{
		/* Step X3. Divide and "Subtract" */
		q = u3 / v3;
		t3 = u3 % v3;
		t1 = u1 + q * v1;
		/* Swap */
		u1 = v1; v1 = t1; u3 = v3; v3 = t3;
		iter = -iter;
	}
	/* Make sure u3 = gcd(u,v) == 1 */
	if (u3 != 1)
		return 0;   /* Error: No inverse exists */
	/* Ensure a positive result */
	if (iter < 0)
		inv = v - u1;
	else
		inv = u1;
	return inv;
}

// Check if the input number is a prime number or not
bool rsa_check_prime(int num) {

	if (num == 0 || num == 1) {
		return false;
	}

	// Return true if the number can only divide 1 and itself
	for (int i = 2; i < num; i++) {
		if (num % i == 0 && i != num) {
			return false;
		}
	}

	return true;
}

// The Modular Exponentiation Algorithm
int rsa_MEA(int p, int e, int n) {

	int r2 = 1;
	int r1 = 0;
	int Q = 0;
	int R = 0;

	while (e != 0) {
		R = (e % 2);
		Q = ((e - R) / 2);

		r1 = ((p * p) % n);

		if (R == 1) {
			r2 = ((r2 * p) % n);
		}
		p = r1;
		e = Q;
	}
	return r2;
}

void rsa_generate_keys(_rsa_data* rsa_data)
{
	rsa_data->p = rsa_genprime();

	do rsa_data->q = rsa_genprime();
	while (rsa_data->q == rsa_data->p);

	rsa_data->n = rsa_data->p * rsa_data->q;
	rsa_data->phi = (rsa_data->p - 1) * (rsa_data->q - 1);

	rsa_data->d = 0;

	while (!rsa_data->d)
	{
		rsa_data->e = rsa_genprime();
		rsa_data->d = rsa_mod_inverse(rsa_data->e, rsa_data->phi);
	}
}

void rsa_get_public(_rsa_data* rsa_data, _rsa_public* key)
{
	key->n = rsa_data->n;
	key->e = rsa_data->e;
}

void rsa_get_private(_rsa_data* rsa_data, _rsa_private* key)
{
	key->n = rsa_data->n;
	key->d = rsa_data->d;
}

int* rsa_encrypt(_rsa_public* key, char* buffer, int size)
{
	int* enc = (int*)malloc(size * sizeof(int));

	for (int i = 0; i < size; ++i)
		enc[i] = rsa_MEA(buffer[i], key->e, key->n);

	return enc;
}

char* rsa_decrypt(_rsa_private* key, int* buffer, int size)
{
	char* dec = (char*)malloc(size * sizeof(char));

	for (int i = 0; i < size; ++i)
		dec[i] = rsa_MEA(buffer[i], key->d, key->n);

	return dec;
}