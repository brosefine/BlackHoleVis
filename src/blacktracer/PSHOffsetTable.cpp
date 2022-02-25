#include <blacktracer/PSHOffsetTable.h>

/* ------------------------------------------------------------------------------------
* Source Code adapted from A.Verbraeck's Blacktracer Black-Hole Visualization
* https://github.com/annemiekie/blacktracer
* https://doi.org/10.1109/TVCG.2020.3030452
* ------------------------------------------------------------------------------------
*/

#include <time.h>
#include <iostream>
#include <fstream>
// ------------- public --------------

PSHOffsetTable::PSHOffsetTable(std::vector<glm::ivec2>& _elements, std::vector<glm::vec2>& _datapoints)
{
	std::srand(time(NULL));

	int size = _elements.size();
	n = size;
	hashTableWidth = calcHashTableWidth(size);
	offsetTableWidth = calcOffsetTableWidth(size);

	hashFilled = std::vector<bool>(hashTableWidth * hashTableWidth);
	hashTable = std::vector<float>(hashTableWidth * hashTableWidth * 2);
	hashPosTag = std::vector<int>(hashTableWidth * hashTableWidth * 2);

	offsetBuckets = std::vector<OffsetBucket>(offsetTableWidth * offsetTableWidth);

	offsetTable = std::vector<int>(offsetTableWidth * offsetTableWidth * 2);
	clearOffsstsToZero();
	elements = _elements;
	datapoints = _datapoints;

	calculateOffsets();
}

void PSHOffsetTable::quicksort(std::vector<OffsetBucket>& bucketList, int start, int end)
{
	int i = start;
	int j = end;
	int pivot = bucketList[start + (end - start) / 2].contents.size();
	while (i <= j) {
		while (bucketList[i].contents.size() > pivot) {
			i++;
		}
		while (bucketList[j].contents.size() < pivot) {
			j--;
		}
		if (i <= j) {
			OffsetBucket temp = bucketList[i];
			bucketList[i] = bucketList[j];
			bucketList[j] = temp;
			i++;
			j--;
		}

	}
	if (start < j)
		quicksort(bucketList, start, j);
	if (i < end)
		quicksort(bucketList, i, end);
}

void PSHOffsetTable::writeToFile(std::string const& fileName) const
{
	std::ofstream ofs(fileName);
	for (auto const& elem : hashTable)
		ofs << ", " << elem;
	ofs.close();
}

glm::ivec2 PSHOffsetTable::hashFunc(glm::ivec2 key)
{
	glm::ivec2 index = hash1(key);
	glm::ivec2 add = { hash0(key).x + offsetTable[(index.x * offsetTableWidth + index.y) * 2],
				 hash0(key).y + offsetTable[(index.x * offsetTableWidth + index.y) * 2 + 1] };
	return hash0(add);
}

// ------------- private --------------

int PSHOffsetTable::calcHashTableWidth(int size)
{
	float d = (float)pow(size * 1.1f, 1.f / 2.f);
	return (int)(d + 1.1f);
}

int PSHOffsetTable::gcd(int a, int b)
{
	if (b == 0)
		return a;
	return gcd(b, a % b);
}

int PSHOffsetTable::calcOffsetTableWidth(int size)
{
	float d = (float)pow(size / 4.f, 1.f / 2.f);
	int width = (int)(d + 1.1f);

	while (gcd(width, hashTableWidth) > 1) { //make sure there are no common factors
		width++;
	}
	return width;
}

void PSHOffsetTable::tryCreateAgain()
{
	creationAttempts++;
	if (creationAttempts >= tableCreateLimit) {
		std::cout << "WRONG" << std::endl;
	}
	resizeOffsetTable();
	clearFilled();
	calculateOffsets();
}

bool PSHOffsetTable::checkForBadCollisions(OffsetBucket bucket)
{
	std::vector<glm::ivec2> testList;
	for (int i = 0; i < bucket.contents.size(); i++) {
		glm::ivec2 ele = bucket.contents[i];
		glm::ivec2 hash = hash0(ele);
		for (int q = 0; q < testList.size(); q++) {
			if (testList[q].x == hash.x && testList[q].y == hash.y) {
				return true;
			}
		}
		testList.push_back(hash);
	}
	return false;
}

void PSHOffsetTable::fillHashCheck(OffsetBucket bucket, glm::ivec2 offset)
{
	for (int i = 0; i < bucket.contents.size(); i++) {
		glm::ivec2 ele = bucket.contents[i];
		glm::ivec2 _hash = hashFunc(ele, offset);
		if (hashFilled[_hash.x * hashTableWidth + _hash.y]) std::cout << "ALREADY FILLED" << std::endl;
		hashFilled[_hash.x * hashTableWidth + _hash.y] = true;
		hashTable[(_hash.x * hashTableWidth + _hash.y) * 2] = bucket.data[i].x;
		hashTable[(_hash.x * hashTableWidth + _hash.y) * 2 + 1] = bucket.data[i].y;

		hashPosTag[(_hash.x * hashTableWidth + _hash.y) * 2] = ele.x;
		hashPosTag[(_hash.x * hashTableWidth + _hash.y) * 2 + 1] = ele.y;

		// Fill hashtable itself as well over here
	}
}

bool PSHOffsetTable::findBadOffset(glm::ivec2 index, std::vector<glm::ivec2>& badOffsets, OffsetBucket bucket, glm::ivec2& offset)
{
	index = hash1(index);
	offset = { offsetTable[(index.x * offsetTableWidth + index.y) * 2],
			   offsetTable[(index.x * offsetTableWidth + index.y) * 2 + 1] };
	for (int q = 0; q < badOffsets.size(); q++) {
		if (badOffsets[q].x == offset.x && badOffsets[q].y == offset.y)
			return false;
	}
	if (OffsetWorks(bucket, offset)) {
		return true;
	}
	badOffsets.push_back(offset);
	return false;
}

bool PSHOffsetTable::findOffsetRandom(OffsetBucket bucket, glm::ivec2& newOffset)
{
	if (bucket.contents.size() == 1) {
		glm::ivec2 hashIndex;
		if (findAEmptyHash(hashIndex)) {
			newOffset = { hashIndex.x - hash0(bucket.contents[0]).x, hashIndex.y - hash0(bucket.contents[0]).y };
			return true;
		}
		else return false;
	}

	glm::ivec2 seed = { (rand() % hashTableWidth) - hashTableWidth / 2,
				  (rand() % hashTableWidth) - hashTableWidth / 2 };
	for (int i = 0; i <= 5; i++) {
		for (int x = i; x < hashTableWidth; x += 5) {
			for (int y = i; y < hashTableWidth; y += 5) {
				glm::ivec2 offset = { seed.x + x, seed.y + y };
				if (OffsetWorks(bucket, offset)) {
					newOffset = offset;
					return true;
				}
				//glm::ivec2 index = hash0(index);
				//if (!hashFilled[index.x * hashTableWidth + index.y]) {
				//	glm::ivec2 offset = { index.x - hash0(bucket.contents[0]).x, index.y - hash0(bucket.contents[0]).y };
			}
		}
	}

	return false;
}

bool PSHOffsetTable::findAEmptyHash(glm::ivec2& index)
{
	glm::ivec2 seed = { (rand() % hashTableWidth) - hashTableWidth / 2, (rand() % hashTableWidth) - hashTableWidth / 2 };
	for (int x = 0; x < hashTableWidth; x++) {
		for (int y = 0; y < hashTableWidth; y++) {
			index = { seed.x + x, seed.y + y };
			index = hash0(index);
			if (!hashFilled[index.x * hashTableWidth + index.y]) return true;
		}
	}
	return false;
}

glm::ivec2 PSHOffsetTable::findAEmptyHash2(glm::ivec2 start)
{
	for (int x = 0; x < hashTableWidth; x++) {
		for (int y = 0; y < hashTableWidth; y++) {
			if (x + y == 0) continue;
			glm::ivec2 index = { start.x + x, start.y + y };
			index = hash0(index);
			if (!hashFilled[index.x * hashTableWidth + index.y]) return index;
		}
	}
	return { -1, -1 };
}

bool PSHOffsetTable::OffsetWorks(OffsetBucket bucket, glm::ivec2 offset)
{
	for (int i = 0; i < bucket.contents.size(); i++) {
		glm::ivec2 ele = bucket.contents[i];
		glm::ivec2 _hash = hashFunc(ele, offset);
		if (hashFilled[_hash.x * hashTableWidth + _hash.y]) {
			return false;
		}
	}
	return true;
}

glm::ivec2 PSHOffsetTable::hash1(glm::ivec2 key)
{
	return{ (key.x + offsetTableWidth) % offsetTableWidth, (key.y + offsetTableWidth) % offsetTableWidth };
}

glm::ivec2 PSHOffsetTable::hash0(glm::ivec2 key)
{
	return{ (key.x + hashTableWidth) % hashTableWidth, (key.y + hashTableWidth) % hashTableWidth };
}

glm::ivec2 PSHOffsetTable::hashFunc(glm::ivec2 key, glm::ivec2 offset)
{
	glm::ivec2 add = { hash0(key).x + offset.x, hash0(key).y + offset.y };
	return hash0(add);
}

void PSHOffsetTable::resizeOffsetTable()
{
	offsetTableWidth += 5; //test
	while (gcd(offsetTableWidth, hashTableWidth % offsetTableWidth) > 1) {
		offsetTableWidth++;
	}
	offsetBuckets = std::vector<OffsetBucket>(offsetTableWidth * offsetTableWidth);
	offsetTable = std::vector<int>(offsetTableWidth * offsetTableWidth * 2);
	clearOffsstsToZero();
}

void PSHOffsetTable::clearFilled()
{
	for (int x = 0; x < hashTableWidth; x++) {
		for (int y = 0; y < hashTableWidth; y++) {
			hashFilled[x * hashTableWidth + y] = false;
		}
	}
}

void PSHOffsetTable::clearOffsstsToZero()
{
	for (int x = 0; x < offsetTableWidth; x++) {
		for (int y = 0; y < offsetTableWidth; y++) {
			offsetTable[(x * offsetTableWidth + y) * 2] = 0;
			offsetTable[(x * offsetTableWidth + y) * 2 + 1] = 0;
		}
	}
}

void PSHOffsetTable::putElementsIntoBuckets()
{
	for (int i = 0; i < n; i++) {
		glm::ivec2 ele = elements[i];
		glm::ivec2 index = hash1(ele);
		if (!offsetBuckets[index.x * offsetTableWidth + index.y].create) {
			offsetBuckets[index.x * offsetTableWidth + index.y] = OffsetBucket(index);
		}
		offsetBuckets[index.x * offsetTableWidth + index.y].contents.push_back(ele);
		offsetBuckets[index.x * offsetTableWidth + index.y].data.push_back(datapoints[i]);
	}
}

std::vector<OffsetBucket> PSHOffsetTable::createSortedBucketList()
{
	std::vector<OffsetBucket> bucketList;

	for (int x = 0; x < offsetTableWidth; x++) { //put the buckets into the bucketlist and sort
		for (int y = 0; y < offsetTableWidth; y++) {
			if (offsetBuckets[x * offsetTableWidth + y].create) {
				bucketList.push_back(offsetBuckets[x * offsetTableWidth + y]);
			}
		}
	}
	quicksort(bucketList, 0, bucketList.size() - 1);
	return bucketList;
}

void PSHOffsetTable::calculateOffsets()
{
	putElementsIntoBuckets();
	std::vector<OffsetBucket> bucketList = createSortedBucketList();

	for (int i = 0; i < bucketList.size(); i++) {
		OffsetBucket bucket = bucketList[i];
		glm::ivec2 offset;
		//if (checkForBadCollisions(bucket)) {
		//	std::cout << "badcollisions" << std::endl;
		//}

		bool succes = findOffsetRandom(bucket, offset);

		if (!succes) {
			tryCreateAgain();
			break;
		}
		offsetTable[(bucket.index.x * offsetTableWidth + bucket.index.y) * 2] = offset.x;
		offsetTable[(bucket.index.x * offsetTableWidth + bucket.index.y) * 2 + 1] = offset.y;

		fillHashCheck(bucket, offset);

	}
}
