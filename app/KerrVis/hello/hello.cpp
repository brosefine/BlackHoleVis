#include <iostream>

#include <blacktracer/Grid.h>

#include <cereal/access.hpp>


struct MyClass
{
private:
    int x, y, z;
    // This method lets cereal know which data members to serialize

    friend class cereal::access; 
    template<class Archive>
    void serialize(Archive& archive)
    {
        archive(x, y, z); // serialize things by passing them to the archive
    }
};

int main()
{
    std::cout << "Hello World! hi\n";
}