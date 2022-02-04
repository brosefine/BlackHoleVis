#pragma once

/* ------------------------------------------------------------------------------------ 
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------ 
*/

#include <vector>
#include <glm/glm.hpp>

#include "cereal/types/vector.hpp"
#include "cereal/types/memory.hpp"
#include "cereal/access.hpp"


/**
* Created by Thomas on 8/9/14.
* This class is used to create perfect spatial hashing for a set of 3D indices.
* This class takes as input a list of 3d index (3D integer vector), and creates a mapping that can be used to pair a 3d index with a value.
* The best part about this type of hash map is that it can compress 3d spatial data in such a way that there spatial coherency (3d indices near each other have paired values near each other in the hash table) and the lookup time is O(1).
* Since it's perfect hashing there is no hash collisions
* This hashmap could be used for a very efficient hash table on the GPU due to coherency and only 2 lookups from a texture-hash-table would be needed, one for the offset to help create the hash, and one for the actual value indexed by the final hash.
* This implementation is based off the paper: http://hhoppe.com/perfecthash.pdf, Perfect Spatial Hashing by Sylvain Lefebvre &Hugues Hopp, Microsoft Research
*
*  To use:
*  accumulate your spatial data in a list to pass to the PSHOffsetTable class
*  construct the table with the list
*  you now use this class just as your "mapping", it has the hash function for your hash table
*  create your 3D hash with the chosen width from PSHOffsetTable.hashTableWidth.
*  Then to get the index into your hash table, just use PSHOffsetTable.hash(key).
*  That's it.
*
*  If you want to update the offsetable, you can do so by using the updateOffsets() with the modified list of spatial data.
*/

class OffsetBucket {
public:
	bool create = false;
	std::vector<glm::ivec2> contents;
	std::vector<glm::vec2> data;
	glm::ivec2 index; //index in offset table
	OffsetBucket() {};

	OffsetBucket(glm::ivec2 _index) {
		index = _index;
		create = true;
	}
};

class PSHOffsetTable {
#pragma region cereal
	friend class cereal::access;
	template < class Archive >
	void serialize(Archive& ar) {
		ar(hashTable, offsetTable, hashPosTag, hashTableWidth, offsetTableWidth);
	}
#pragma endregion

public:
	std::vector<int> offsetTable; // used to be [][]
	std::vector<float> hashTable;
	std::vector<int> hashPosTag;

	int offsetTableWidth;
	int hashTableWidth;
	int n;

	glm::ivec2 hashFunc(glm::ivec2 key);

	PSHOffsetTable() {};

	PSHOffsetTable(std::vector<glm::ivec2>& _elements, std::vector<glm::vec2>& _datapoints);

	void quicksort(std::vector<OffsetBucket>& bucketList, int start, int end);

	void writeToFile(std::string const& fileName) const;

private:

	std::vector<glm::ivec2> elements;
	std::vector<glm::vec2> datapoints;
	std::vector<OffsetBucket> offsetBuckets;
	std::vector<bool> hashFilled;

	int offsetFindLimit = 120;
	int tableCreateLimit = 10;
	int creationAttempts = 0;

	int calcHashTableWidth(int size);

	int gcd(int a, int b);

	int calcOffsetTableWidth(int size);

	void tryCreateAgain();

	bool checkForBadCollisions(OffsetBucket bucket);

	void fillHashCheck(OffsetBucket bucket, glm::ivec2 offset);

	bool findBadOffset(glm::ivec2 index, std::vector<glm::ivec2>& badOffsets, OffsetBucket bucket, glm::ivec2& offset);

	bool findOffsetRandom(OffsetBucket bucket, glm::ivec2& newOffset);

	bool findAEmptyHash(glm::ivec2& index);

	glm::ivec2 findAEmptyHash2(glm::ivec2 start);

	bool OffsetWorks(OffsetBucket bucket, glm::ivec2 offset);

	glm::ivec2 hash1(glm::ivec2 key);

	glm::ivec2 hash0(glm::ivec2 key);

	glm::ivec2 hashFunc(glm::ivec2 key, glm::ivec2 offset);

	void resizeOffsetTable();

	void clearFilled();

	void clearOffsstsToZero();

	void cleanUp() {
		//elements = 0;
		//offsetBuckets = 0;
		//hashFilled = 0;
	}

	void putElementsIntoBuckets();

	std::vector<OffsetBucket> createSortedBucketList();

	void calculateOffsets();


};
