
long long a = 0;
void infinite(long long b)
{
    a = b;
    infinite(b + 1);
}

extern "C" int main(int argc, const char **argv)
{
    //infinite(0);
    return 0;
}