
class classBase
    {
    public:
        void func(int &a);
    };

class classMultiLeaf
    {
    public:
        classMultiLeaf():
            classMultiLeaf_intMember(0)
            {}

    private:
        int classMultiLeaf_intMember;
    };

class classLeaf3a
    {
    public:
        classLeaf3a():
            classLeaf3a_leaf3aMember(0)
            {}

    private:
        int classLeaf3a_leaf3aMember;
        classMultiLeaf classLeaf3a_multiLeafMember;
    };

