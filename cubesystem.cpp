#include "cubesystem.hpp"

void CubeSystem::Print(Matrix& m)
{
	for (int i = 0; i < m.rowdim(); ++i)
	{
		std::cout << "[ ";
		for (int j = 0; j < m.coldim(); ++j)
		{
			std::cout << m.getEntry(i, j) << " ";
		}
		std::cout << "]" << std::endl;
	}
	std::cout << std::endl;
}

std::vector<int> CubeSystem::CanonicalCoordinates(Chain& c, std::vector<Cube>& cubes)
{
	std::vector<int> v;

	// go through the array of cubes and evalute the chain
	// on them. collect the coefficients and put them in the 
	// vector.
	for (int i = 0; i < cubes.size(); ++i)
	{
		auto it = c.find(cubes[i]);
		if (it == c.end())
		{
			v.push_back(0);	
		}
		else
		{
			v.push_back(it->second);
		}
	}

	return v;
}

Chain CubeSystem::ChainFromCanonicalCoordinates(std::vector<int>& v, std::vector<Cube>& cubes)
{
	Chain c;
	for (int i = 0; i < cubes.size(); ++i)
	{
		if (v[i] != 0)
		{
			c[cubes[i]] = v[i];	
		}
	}
	return c;
}


std::unordered_map<Cube, int, KeyHasher> CubeSystem::PrimaryFaces(Cube& Q)
{
	std::unordered_map<Cube, int, KeyHasher> faces;

	for (int i = 0; i < Q.EmbeddingNumber(); ++i)
	{
		if (!(Q[i].isDegenerate()))
		{
			Cube R = Q;

			R[i].setLeft(Q[i].getLeft());
			R[i].setRight(Q[i].getLeft());
			//R.Print();
			faces[R] = R.Dimension();

			R[i].setLeft(Q[i].getRight());
			R[i].setRight(Q[i].getRight());
			//R.Print();
			faces[R] = R.Dimension();
		}
	}

	return faces;
}

std::vector<Cube> CubeSystem::GetCoordinates(std::unordered_map<Cube, int, KeyHasher>& map)
{
	std::vector<Cube> cubes;
	cubes.reserve(map.size());
	for (auto it : map)
	{
		cubes.push_back(it.first);
	}
	return cubes;
}

std::vector<std::unordered_map<Cube, int, KeyHasher>> CubeSystem::CubicalChainGroups(CubicalSet& K)
{
	std::cout << "Dimension of the cubical set K: " << K.Dimension() << std::endl;
	std::vector<std::unordered_map<Cube, int, KeyHasher>> E(K.Dimension() + 1);	

	std::cout << "Number of chain groups to consider: " << E.size() << std::endl;
	
	while (!K.isEmpty())
	{
		// test:
		//std::cout << "Size of K: " << K.cubes.size() << std::endl;

		Cube Q = K.Pop();

		//std::cout << "The current cube being considered in K is: " << std::endl;
		//Q.Print();
		//std::cout << "with dimension: " << Q.Dimension() << std::endl;


		int k = Q.Dimension();
		if (k > 0)
		{
			std::unordered_map<Cube, int, KeyHasher> L = PrimaryFaces(Q);

			// union of K and L:
			K.cubes.insert(L.begin(), L.end());

			// L gives (k-1)-dimension cubes. so they go in
			// E[k-1]:
			E[k-1].insert(L.begin(), L.end());
		}

		// Q is k-dimensional so it goes in E[k]:
		E[k].insert(std::pair<Cube, int>(Q, Q.Dimension()));

	}

	return E;
}

Chain CubeSystem::BoundaryOperator(Cube& Q)
{
	int sign = 1;
	Chain c;

	for (int i = 0; i < Q.size(); ++i)
	{
		if (!Q[i].isDegenerate())
		{
			Cube R = Q;
			R[i].setLeft(Q[i].getLeft());
			R[i].setRight(Q[i].getLeft());
			c[R] = -sign;

			R[i].setLeft(Q[i].getRight());
			R[i].setRight(Q[i].getRight());
			c[R] = sign;

			sign = -sign;
		}
	}

	return c;
}

BoundaryMap CubeSystem::Boundaries(ChainComplex& E)
{
	// bd is an arrray where bd[k] is the boundary map from k+1 to k,
	// since we are skipping zero.
	BoundaryMap bd(E.size() - 1);

	for (int k = 1; k < E.size(); ++k)
	{
		// evaluate the boundary operator on each cube of E[k]:
		for (int j = 0; j < E[k].size(); ++j)
		{
			Cube& e = E[k][j];
			Chain c = BoundaryOperator(e);

			bd[k-1][e] = c;
		}
	}

	return bd;
}

// LinBox version.
std::vector<Matrix> CubeSystem::BoundaryOperatorMatrixLinBox(std::vector<std::vector<Cube>>& E, BoundaryMap& bd)
{
	std::vector<Matrix> matrices;

	for (int k = 1; k < E.size(); ++k)
	{
		// bd: K_k --> K_{k-1}.
		int lastRow = E[k-1].size() - 1;
		int lastColumn = E[k].size() - 1;

		Matrix matrix(ZZ, lastRow + 1, lastColumn + 1);
		std::cout << "Computing the " << k << "th boundary matrix. " << std::endl;


		// evaluate the boundary operator on each cube of E[k]:
		for (int j = 0; j < E[k].size(); ++j)
		{
			Cube& e = E[k][j];
			Chain c = bd[k-1][e];
			std::vector<int> column = CanonicalCoordinates(c, E[k-1]);

			// Set this as the jth column of the matrix, but only nonzero entries.
			for (int i = 0; i < column.size(); ++i)
			{
				if (column[i] != 0)
					matrix.setEntry(i, j, column[i]);
			}
		}
		matrix.finalize();
		std::cout << "Built boundary matrix with size " << matrix.size() << " and dimension ";
		std::cout << matrix.rowdim() << " x " << matrix.coldim() << std::endl;

		matrices.push_back(matrix);
	}
		
	// Export the 0th boundary matrix to a file for testing.
	/*
	std::ofstream output("output.txt");
	if (output.is_open())
	{
		matrices[0].write(output);
		output.close();
	}
	*/

	return matrices;
}
std::vector<IntMat> CubeSystem::BoundaryOperatorMatrix(std::vector<std::vector<Cube>>& E, BoundaryMap& bd)
{
	std::vector<IntMat> matrices;

	for (int k = 1; k < E.size(); ++k)
	{
		// bd: K_k --> K_{k-1}.
		int lastRow = E[k-1].size() - 1;
		int lastColumn = E[k].size() - 1;

		IntMat matrix(lastRow + 1, lastColumn + 1);

		// evaluate the boundary operator on each cube of E[k]:
		for (int j = 0; j < E[k].size(); ++j)
		{
			Cube& e = E[k][j];
			Chain c = bd[k-1][e];
			std::vector<int> column = CanonicalCoordinates(c, E[k-1]);
			matrix.setColumn(j, column);
		}

		matrices.push_back(matrix);
	}

	return matrices;
}
std::vector<Matrix> CubeSystem::BoundaryOperatorMatrixLinBox(std::vector<std::vector<Cube>>& E)
{
	std::vector<Matrix> matrices;

	for (int k = 1; k < E.size(); ++k)
	{
		// bd: K_k --> K_{k-1}.
		int lastRow = E[k-1].size() - 1;
		int lastColumn = E[k].size() - 1;

		Matrix matrix(ZZ, lastRow + 1, lastColumn + 1);

		// evaluate the boundary operator on each cube of E[k]:
		for (int j = 0; j < E[k].size(); ++j)
		{
			Chain c = BoundaryOperator(E[k][j]);
			std::vector<int> column = CanonicalCoordinates(c, E[k-1]);

			// Set this as the jth column of the matrix.
			for (int i = 0; i < column.size(); ++i)
			{
				matrix.setEntry(i, j, column[i]);
			}
		}

		matrices.push_back(matrix);
	}

	return matrices;
}
std::vector<IntMat> CubeSystem::BoundaryOperatorMatrix(std::vector<std::vector<Cube>>& E)
{
	std::vector<IntMat> matrices;

	for (int k = 1; k < E.size(); ++k)
	{
		// bd: K_k --> K_{k-1}.
		int lastRow = E[k-1].size() - 1;
		int lastColumn = E[k].size() - 1;

		IntMat matrix(lastRow + 1, lastColumn + 1);

		// evaluate the boundary operator on each cube of E[k]:
		for (int j = 0; j < E[k].size(); ++j)
		{
			Chain c = BoundaryOperator(E[k][j]);
			std::vector<int> column = CanonicalCoordinates(c, E[k-1]);
			matrix.setColumn(j, column);
		}

		matrices.push_back(matrix);
	}

	return matrices;
}


std::vector<std::vector<int>> CubeSystem::GetHomologyLinBox(CubicalSet& K, bool CCR)
{
	// get the generators for C_k:
	std::vector<std::unordered_map<Cube, int, KeyHasher>> chainGroups = CubicalChainGroups(K);
	
	// convert the generators into coordinates:
	std::vector<std::vector<Cube>> E;
	for (int i = 0; i < chainGroups.size(); ++i)
	{
		E.push_back(GetCoordinates(chainGroups[i]));
	}

	//std::vector<IntMat> D;
	std::vector<Matrix> matrices;

	if (CCR)
	{
		// get the boundary operators:
		BoundaryMap bd = Boundaries(E);

		// apply the CCR algorithm:
		std::cout << "Size of chain groups before CCR:" << std::endl;
		for (int i = 0; i < E.size(); ++i)
		{
			std::cout << E[i].size() << std::endl;
		}
		ReduceChainComplex(E, bd);		
		std::cout << "Size of chain groups after CCR:" << std::endl;
		for (int i = 0; i < E.size(); ++i)
		{
			std::cout << E[i].size() << std::endl;
		}
	
		// get the boundary operator matrices from the chains:
		//D = BoundaryOperatorMatrix(E, bd);
		matrices = BoundaryOperatorMatrixLinBox(E, bd);
	}
	else
	{
		// get the boundary operator matrices from the chains:
		//D = BoundaryOperatorMatrix(E);
		matrices = BoundaryOperatorMatrixLinBox(E);
	}

	// compute the homology groups:
	/*
	std::vector<std::vector<int>> homLinBox = Homology::GetHomologyLinBox(matrices);
	for (int i = 0; i < homLinBox.size(); ++i)
	{
		for (int j = 0; j < homLinBox[i].size(); ++j)
		{
			std::cout << homLinBox[i][j] << " ";
		}
		std::cout << std::endl;
	}
	*/
	std::vector<std::vector<int>> homologies = Homology::GetHomologyValence(matrices);
	for (int i = 0; i < homologies.size(); ++i)
	{
		for (int j = 0; j < homologies[i].size(); ++j)
		{
			std::cout << homologies[i][j] << " ";
		}
		std::cout << std::endl;
	}
	return homologies;
}

std::vector<std::vector<int>> CubeSystem::GetHomology(CubicalSet& K, bool CCR)
{
	// get the generators for C_k:
	std::vector<std::unordered_map<Cube, int, KeyHasher>> chainGroups = CubicalChainGroups(K);
	
	// convert the generators into coordinates:
	std::vector<std::vector<Cube>> E;
	for (int i = 0; i < chainGroups.size(); ++i)
	{
		E.push_back(GetCoordinates(chainGroups[i]));
	}

	std::vector<IntMat> D;

	if (CCR)
	{
		// get the boundary operators:
		BoundaryMap bd = Boundaries(E);

		// apply the CCR algorithm:
		ReduceChainComplex(E, bd);		
	
		// get the boundary operator matrices from the chains:
		D = BoundaryOperatorMatrix(E, bd);
	}
	else
	{
		// get the boundary operator matrices from the chains:
		D = BoundaryOperatorMatrix(E);
	}

	// compute the homology groups:
	std::vector<std::vector<int>> hom = Homology::GetHomology(D);
	for (int i = 0; i < hom.size(); ++i)
	{
		for (int j = 0; j < hom[i].size(); ++j)
		{
			std::cout << hom[i][j] << " ";
		}
		std::cout << std::endl;
	}
	return hom;
}

void CubeSystem::Homology(CubicalSet& K, bool CCR)
{
	// get the generators for C_k:
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	std::vector<std::unordered_map<Cube, int, KeyHasher>> chainGroups = CubicalChainGroups(K);
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << "Time to create chain groups: " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() / 1000000.0 << " seconds." << std::endl;


	
	// convert the generators into coordinates:
	begin = std::chrono::steady_clock::now();
	std::vector<std::vector<Cube>> E;
	for (int i = 0; i < chainGroups.size(); ++i)
	{
		E.push_back(GetCoordinates(chainGroups[i]));
	}
	end = std::chrono::steady_clock::now();
	std::cout << "Time to convert chains to coordinates: " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() / 1000000.0 << " seconds." << std::endl;

	std::vector<IntMat> D;
	if (CCR)
	{
		// get the boundary operators:
		begin = std::chrono::steady_clock::now();
		BoundaryMap bd = Boundaries(E);
		end = std::chrono::steady_clock::now();
		std::cout << "Time to create boundary maps: " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() / 1000000.0 << " seconds." << std::endl;

		// apply the CCR algorithm:
		begin = std::chrono::steady_clock::now();
		ReduceChainComplex(E, bd);		
		end = std::chrono::steady_clock::now();
		std::cout << "Time to do CCR: " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() / 1000000.0 << " seconds." << std::endl;

	
		// get the boundary operator matrices from the chains:
		begin = std::chrono::steady_clock::now();
		D = BoundaryOperatorMatrix(E, bd);
		end = std::chrono::steady_clock::now();
		std::cout << "Time to get boundary matrices: " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() / 1000000.0 << " seconds." << std::endl;
		std::cout << "Sizes of the matrices: " << std::endl;
		for (int i = 0; i < D.size(); ++i)
		{
			std::cout << D[i].getRows() << " x " << D[i].getColumns() << std::endl;
		}
	}
	else
	{
		// get the boundary operator matrices from the chains:
		begin = std::chrono::steady_clock::now();
		D = BoundaryOperatorMatrix(E);
		end = std::chrono::steady_clock::now();
		std::cout << "Time to get boundary matrices: " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() / 1000000.0 << " seconds." << std::endl;
		std::cout << "Sizes of the matrices: " << std::endl;
		for (int i = 0; i < D.size(); ++i)
		{
			std::cout << D[i].getRows() << " x " << D[i].getColumns() << std::endl;
		}
	}

	// compute the homology groups:
	//begin = std::chrono::steady_clock::now();
	//std::vector<std::vector<int>> hom = Homology::GetHomology(D);
	//Homology::AnalyzeHomology(hom);
	//end = std::chrono::steady_clock::now();
	//std::cout << "Time to compute homology: " << std::chrono::duration_cast<std::chrono::microseconds>(end-begin).count() / 1000000.0 << " seconds." << std::endl;




	//std::vector<Quotient> H = Homology::HomologyGroupOfChainComplex(D);

	// analyze the homology groups:
	//Homology::AnalyzeHomology(H);
}

/*
void CubeSystem::Reduce(ChainComplex& E, BoundaryMap& bd, int i, Cube& a, Cube& b)
{
	std::cout << "i = " << i << std::endl;
	for (Cube cube : E[i])
	{
		bd[i-1][cube].erase(b);
	}

	for (Cube cube : E[i-1])
	{
		//if (bd[i-2][cube].find(a) != bd[i-2][cube].end())
		//{
			for (auto it : bd[i-1][b])
			{
				//std::cout << bd[i-1][cube][it.first] << std::endl;
				bd[i-1][cube][it.first] -= bd[i-1][cube][a] * bd[i-1][b][a] * bd[i-1][b][it.first];
				//std::cout << bd[i-1][cube][it.first] << std::endl;
			}
		//}
	}

	RemoveElementFromVector(E[i], b);		
	RemoveElementFromVector(E[i - 1], a);		
	bd[i-1].erase(b);
	bd[i-2].erase(a);
}
*/
void CubeSystem::Reduce(ChainComplex& E, BoundaryMap& bd, int i, Cube& a, Cube& b)
{
	/*
	std::cout << "*************************" << std::endl;
	std::cout << "Cube b: " << std::endl;
	b.Print();
	std::cout << "Cube a: " << std::endl;
	a.Print();
	*/

	// Remove b as a boundary of all (i+1)-dim cubes.
	if (i < E.size() - 1)
	{
		for (Cube cube : E[i+1])
		{
			if (bd[i][cube].find(b) != bd[i][cube].end())
			{
				/*
				std::cout << "Erasing b from cube: " << std::endl;
				cube.Print();
				*/
				bd[i][cube].erase(b);
			}
		}
	}

	// Update the other affected i-dim cubes.
	for (Cube cube : E[i])
	{
		if (cube == b)
			continue;

		// Only update if a was attached to cube.
		if (bd[i-1][cube].find(a) != bd[i-1][cube].end())
		{
			/*
			std::cout << "** Found cube attached to a: " << std::endl;
			cube.Print();
			*/

			// For all cubes attached to b, update:
			for (auto it : bd[i-1][b])
			{
				const Cube& c = it.first;
				/*
				std::cout << "* Recalculating boundary of cube: " << std::endl;
				c.Print();
				*/

				/*
				std::cout << "Original value: ";
				std::cout << bd[i-1][cube][c] << ". ";
				std::cout << "New value: ";
				std::cout << bd[i-1][cube][c] << ". " << std::endl;
				*/
				bd[i-1][cube][c] -= bd[i-1][cube][a] * bd[i-1][b][a] * bd[i-1][b][c];
			}
		}
	}

	RemoveElementFromVector(E[i], b);		
	RemoveElementFromVector(E[i - 1], a);		
	bd[i-1].erase(b);
	bd[i-2].erase(a);
}

void CubeSystem::RemoveElementFromVector(std::vector<Cube>& v, Cube& e)
{
	auto it = std::find(v.begin(), v.end(), e);

	// then the item has been found.
	if (it != v.end())
	{
		// swap the found element to the end of the vector and pop it off.
		std::swap(*it, v.back());
		v.pop_back();
	}
}

// WARNING:
// for an as of yet unknown reason, this algorithm breaks
// on 3D objects. possible reasons:
// * the book is wrong about the boundary formula in 3D and higher.
// * I misinterpreted the typo in the book, even though this algorithm does work in 2D.
// * the boundary operator is bugged in 3D (my fault).
// * I just screwed up in general (most likely case).
/*
void CubeSystem::ReduceChainComplex(ChainComplex& E, BoundaryMap& bd)
{
	for (int i = E.size() - 1; i > 1; --i)
	{
		bool found = false;
		while (!found)
		{
			std::cout << "i = " << i << std::endl;
			for (Cube b : E[i])
			{
				for (Cube a : E[i-1])
				{
					//if (bd[i-1].find(b) != bd[i-1].end())
				//	{
					//	if (bd[i-1][b].find(a) != bd[i-1][b].end())
					//	{
							if (std::abs(bd[i-1][b][a] == 1))
							{
								std::cout << "Reducing: " << std::endl;
								a.Print();
								b.Print();

								Reduce(E, bd, i, a, b);
								found = true;
								//break;
							}
					//	}
				//	}
				}
			}
			found = true;
		}
	}
}
*/

// WARNING:
// Bug was fixed for top-dim faces, but there is still an issue for lower-dim faces when the boundary map is changed.
// For now, only perform this algorithm for top-dim faces when we collapse free faces.
void CubeSystem::ReduceChainComplex(ChainComplex& E, BoundaryMap& bd)
{
	// Loop until we've found no more reduction pairs.
	bool reductionPairs = true;
	while (reductionPairs)
	{
		// The index i is the dimension we are looking at.
		// Attempting to reduce an i-dim face by an (i-1)-dim face.
		// Start at i equal to the top dimension, down to i=1 (edges).
		//for (int i = E.size() - 1; i > 1; --i)
		// WARNING: There is currently a bug for non-free faces.
		// Keep the loop only for top-dim faces.
		for (int i = E.size() - 1; i > E.size() - 2; --i)
		{
			bool found = false;
			while (!found)
			{
				// b is an i-dimensional face.
				for (Cube b : E[i])
				{
					if (found)
						break;

					// a is an (i-1)-dimensional face.
					for (Cube a : E[i-1])
					{
						if (found)
							break;
						//if (bd[i-1].find(b) != bd[i-1].end())
					//	{
							if (bd[i-1][b].find(a) != bd[i-1][b].end())
							{
								// Recall bd[i-1] is the boundary from C_i to C_{i-1}, so use bd[i-1](b).
								if (std::abs(bd[i-1][b][a]) == 1)
								{
									Reduce(E, bd, i, a, b);
									found = true;
									break;
								}
							}
					//	}
					}
				}
				// Exit condition if no pairs were found.
				if (!found)
				{
					found = true;
					reductionPairs = false;
				}
			}
		}
	}
}

int CubeSystem::ScalarProduct(Chain& c1, Chain& c2)
{
	int product = 0;
	for (auto it : c1)
	{
		product += c2[it.first] * it.second;
	}
	return product;
}


CubicalSet CubeSystem::GetCubicalSet(Grid& grid)
{
	std::vector<Cube> cubes;

	// go through the grid and create a cube for each activated square.
	// the grid coordinates (x, y) will refer to the bottom left corner
	// of the elementary cube. so each cube will have two intervals:
	// (x, x+1) and (y, y+1).
	
	for (int x = 0; x < grid.getRows(); x++)
	{
		for (int y = 0; y < grid.getColumns(); y++)
		{
			if (grid.getElement(x, y))
			{
				Cube c; 
				c.addInterval(Interval(x));
				c.addInterval(Interval(y));
				cubes.push_back(c);
			}
		}
	}

	return CubicalSet(cubes);
}
CubicalSet CubeSystem::GetCubicalSet(Grid3D& block)
{
	std::vector<Cube> cubes;

	// go through the grid and create a cube for each activated square.
	// the grid coordinates (x, y) will refer to the bottom left corner
	// of the elementary cube. so each cube will have two intervals:
	// (x, x+1) and (y, y+1).
	
	for (int x = 0; x < block.getRows(); ++x)
	{
		for (int y = 0; y < block.getColumns(); ++y)
		{
			for (int z = 0; z < block.getSteps(); ++z)
			{
				if (block.getElement(x, y, z))
				{
					Cube c; 
					c.addInterval(Interval(x));
					c.addInterval(Interval(y));
					c.addInterval(Interval(z));
					cubes.push_back(c);
				}
			}
		}
	}

	return CubicalSet(cubes);
}
