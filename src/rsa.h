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
bool rsa_check_prime(num) {

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

typedef struct
{
	int p;
	int q;

	int n;
	int phi;

	int e;
	int d;
} _rsa_data;

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

bool rsa_save_public_key(_rsa_data* rsa_data, const char* path)
{
	int data[] = { rsa_data->n, rsa_data->e };
	return save_file(data, sizeof(data), path);
}

bool rsa_save_private_key(_rsa_data* rsa_data, const char* path)
{
	int data[] = { rsa_data->n, rsa_data->d };
	return save_file(data, sizeof(data), path);
}

bool rsa_save_keys(_rsa_data* rsa_data, const char* name)
{
	char buffer[128];
	bool status = true;

	sprintf(buffer, "%s.pub", name);
	status &= status && rsa_save_public_key(rsa_data, buffer);

	sprintf(buffer, "%s.priv", name);
	status &= status && rsa_save_private_key(rsa_data, buffer);

	return status;
}

bool rsa_save_client_keys(_rsa_data* rsa_data, int client_id)
{
	char buffer[64];
	sprintf(buffer, "%d", client_id);
	return rsa_save_keys(rsa_data, buffer);
}

bool rsa_load_public_key(_rsa_data* rsa_data, const char* path)
{
	int data[] = { rsa_data->n, rsa_data->e };
	bool status = load_file_static(data, sizeof(data), path);
	if (!status) return false;
	rsa_data->n = data[0];
	rsa_data->e = data[1];
	return true;
}

bool rsa_load_private_key(_rsa_data* rsa_data, const char* path)
{
	int data[] = { rsa_data->n, rsa_data->d };
	bool status = load_file_static(data, sizeof(data), path);
	if (!status) return false;
	rsa_data->n = data[0];
	rsa_data->d = data[1];
	return true;
}

bool rsa_load_keys(_rsa_data* rsa_data, const char* name)
{
	char buffer[128];
	bool status = true;

	sprintf(buffer, "%s.pub", name);
	status &= rsa_load_public_key(rsa_data, buffer);

	sprintf(buffer, "%s.priv", name);
	status &= rsa_load_private_key(rsa_data, buffer);

	return status;
}

bool rsa_load_client_keys(_rsa_data* rsa_data, int client_id)
{
	char buffer[64];
	sprintf(buffer, "%d", client_id);
	return rsa_load_keys(rsa_data, buffer);
}

int* rsa_encrypt(_rsa_data* rsa_data, char* buffer, int size)
{
	int* enc = malloc(size * sizeof(int));

	for (int i = 0; i < size; ++i)
		enc[i] = rsa_MEA(buffer[i], rsa_data->e, rsa_data->n);

	return enc;
}

char* rsa_decrypt(_rsa_data* rsa_data, int* buffer, int size)
{
	char* dec = malloc(size * sizeof(char));

	for (int i = 0; i < size; ++i)
		dec[i] = rsa_MEA(buffer[i], rsa_data->d, rsa_data->n);

	return dec;
}

int test()
{
	srand(time(NULL));

	_rsa_data rsa_data;
	rsa_generate_keys(&rsa_data);
	rsa_save_client_keys(&rsa_data, 11);
	rsa_load_client_keys(&rsa_data, 11);

	char buffer[] = { "hello data ;D" };
	printf("src: %s\n", buffer);

	int* enc  = rsa_encrypt(&rsa_data, buffer, sizeof(buffer) - 1);
	char* dec = rsa_decrypt(&rsa_data, enc, sizeof(buffer) - 1);
	
	printf("dst: %s\n", dec);

	free(enc);
	free(dec);

	return 0;
}