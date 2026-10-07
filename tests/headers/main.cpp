/** Header isolation (tests/headers/CMakeLists.txt): the checks are compile-time; each TU also uses its part */
int useWiring();
int useIpc();
int useBroker();
int useConfig();

int main()
{
    return (useWiring() == 2 && useIpc() == 1 && useBroker() == 1 && useConfig() == 4) ? 0 : 1;
}
