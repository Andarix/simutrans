#pragma once

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Configuration::Install;


[!output SAFE_NAMESPACE_BEGIN]
	[RunInstaller(true)]

	/// <summary>
	/// Zusammenfassung für [!output SAFE_ITEM_NAME]
	/// </summary>
	public ref class [!output SAFE_ITEM_NAME] : public System::Configuration::Install::Installer
	{
	public:
		[!output SAFE_ITEM_NAME](void)
		{
			InitializeComponent();
			//
			//TODO: Konstruktorcode hier hinzufügen.
			//
		}

	protected:
		/// <summary>
		/// Verwendete Ressourcen bereinigen.
		/// </summary>
		~[!output SAFE_ITEM_NAME]()
		{
			if (components)
			{
				delete components;
			}
		}

	private:
		/// <summary>
		/// Erforderliche Designervariable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Erforderliche Methode für die Designerunterstützung.
		/// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
		/// </summary>
		void InitializeComponent(void)
		{
		}
#pragma endregion
	};
[!output SAFE_NAMESPACE_END]
