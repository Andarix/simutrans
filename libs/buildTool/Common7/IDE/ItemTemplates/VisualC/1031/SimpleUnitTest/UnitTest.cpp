#include "stdafx.h"

using namespace System;
using namespace System::Text;
using namespace System::Collections::Generic;
using namespace Microsoft::VisualStudio::TestTools::UnitTesting;

namespace $rootnamespace$
{
	[TestClass]
	public ref class $safeitemname$
	{
	private:
		TestContext^ testContextInstance;

	public: 
		/// <summary>
		///Ruft den Textkontext mit Informationen über
		///den aktuellen Testlauf sowie Funktionalität für diesen auf oder legt diese fest.
		///</summary>
		property Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ TestContext
		{
			Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ get()
			{
				return testContextInstance;
			}
			System::Void set(Microsoft::VisualStudio::TestTools::UnitTesting::TestContext^ value)
			{
				testContextInstance = value;
			}
		};

		#pragma region Additional test attributes
		//
		//Sie können beim Schreiben der Tests folgende zusätzliche Attribute verwenden:
		//
		//Verwenden Sie ClassInitialize, um vor Ausführung des ersten Tests in der Klasse Code auszuführen.
		//[ClassInitialize()]
		//static void MyClassInitialize(TestContext^ testContext) {};
		//
		//Verwenden Sie ClassCleanup, um nach Ausführung aller Tests in einer Klasse Code auszuführen.
		//[ClassCleanup()]
		//static void MyClassCleanup() {};
		//
		//Mit TestInitialize können Sie vor jedem einzelnen Test Code ausführen.
		//[TestInitialize()]
		//void MyTestInitialize() {};
		//
		//Mit TestCleanup können Sie nach jedem Test Code ausführen.
		//[TestCleanup()]
		//void MyTestCleanup() {};
		//
		#pragma endregion 

		[TestMethod]
		void TestMethod1()
		{
			//
			// TODO: Testlogik hier hinzufügen
			//
		};
	};
}
