#include "CBoundingSphere.h";

class CBoundingSphere {

	class C3DPoint {
		float x, y, z;
	};

	class CBoundingSphereParam {
		C3DPoint center;
		float radius;
	};

	class CBoundingBox {

		class CBoundingBoxPara {
			C3DPoint vs[3];
		};

		CBoundingBox calculateBox(C3DPoint vertices[]) {
			C3DPoint min, max;
			int i, j;
			CBoundingBox box;

			//minimale Werte möglichst hoch setzen
			//maximale Werte möglichst niedrig setzen
			min.x = 10000000; max.x = -10000000;
			min.y = 10000000; max.y = -10000000;
			min.z = 10000000; max.z = -10000000;

			//minimale und maximale Werte berechnen
			for (i = 0; i <= vertices.length()-1; i++) {
				if (vertices[i].x < min.x) {
					min.x = x;
				}
				if (vertices[i].x > max.x) {
					max.x = x;
				}
				if (vertices[i].y < min.y) {
					min.y = y;
				}
				if (vertices[i].y > max.y) {
					max.y = y;
				}
				if (vertices[i].z < min.z) {
					min.z = z;
				}
				if (vertices[i].z > max.z) {
					max.z = z;
				}

			}

			if (i = 0; i < 2; i++) {
				if (j = 0; j < 2; j++) {
					box.vs[0, i, j].x = min.x;
					box.vs[1, i, j].x = max.x;
					box.vs[i, 0, j].y = min.y;
					box.vs[i, 1, j].y = max.y;
					box.vs[i, j, 0].z = min.z;
					box.vs[i, j, 1].z = max.z;
				}
			}

		}


	};

	CBoundingSphere calculateBoundingSphere(C3DPoint vertices) {
		int i;
		float dx, dy, dz, le;
		CBoundingBox box;
		CBoundingSphereParam sphere;

		//Bestimmung des Mittelpunktes
		box = CBoundingBox.calculateBox(vertices);
		sphere.center.x = (box.vs[0][0][0].x + box.vs[1][1][1].x) / 2;
		sphere.center.y = (box.vs[0][0][0].y + box.vs[1][1][1].y) / 2;
		sphere.center.z = (box.vs[0][0][0].z + box.vs[1][1][1].z) / 2;

		//längstmöglichsten Abstand vom Schwerpunkt aus ermitteln
		sphere.radius = 0;
		for (i = 0; i <= vertices.length() - 1; i++) {
			dx = vertices[i].x - sphere.center.x;
			dy = vertices[i].y - sphere.center.y;
			dz = vertices[i].z - sphere.center.z;
			le = sqrt(dx*dx + dy*dy + dz*dz);

			if (sphere.radius < le){
				sphere.radius = le;
			}

		}

		return sphere;



	}



};
