program COMMUNICATE_PROG {
    version COMMUNICATE_VERSION
    {
        bool Join (string IP, int Port) = 1;
        bool Leave (string IP, int Port) = 2;
        bool Subscribe(string IP, int Port, string Article) = 3;
        bool Unsubscribe (string IP, int Port, string Article) = 4;
        bool Publish (string Article, string IP, int Port) = 5;
        bool Ping () = 6;
    } = 1;
} = 0x12345678;