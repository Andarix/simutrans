// Copyright (c) Microsoft Corporation. All rights reserved.

function OnPrep(selProj, selObj)
{
	var L_WizardDialogTitle_Text = "Moduldefinitionsdatei-Assistent";
	return PrepCodeWizard(selProj, L_WizardDialogTitle_Text);
}

function OnFinish(selProj, selObj)
{   
	var oCM;
	try
	{
		oCM	= selProj.CodeModel;

		var strTemplatePath = wizard.FindSymbol("TEMPLATES_PATH");
		var strDefFileName = wizard.FindSymbol("ITEM_NAME");
		var L_addFile_Text = "Datei hinzufügen ";
		oCM.StartTransaction( L_addFile_Text + strDefFileName);
	
		// check whether there is already a .def file in the project
		var nPI, strPIName;
		var oFiles = selProj.Object.Files;
		var cFiles = oFiles.Count;
		for (nPI=1; nPI<=cFiles; nPI++)
		{
			strPIName = oFiles(nPI).Name;
			if (strPIName.substr(strPIName.length-4)==".def")
			{	
				L_DefFileExists_Text = "Es kann nur eine .def-Datei pro Projekt vorhanden sein";
				var oError = new Error(L_DefFileExists_Text);
				SetErrorInfo(oError);
				return;
			}
		}

		// use the filesystem object to check for file existance
		var oFSO = new ActiveXObject("Scripting.FileSystemObject");
		if (oFSO.FileExists(strDefFileName))
		{
			L_thisFileExists_Text = " ist bereits vorhanden. Wählen Sie einen anderen Dateinamen aus.";
			wizard.ReportError(strDefFileName + L_thisFileExists_Text);
			return VS_E_WIZCANCEL;
		}
		
		// render the def file 
		var oLinker = selProj.Object.Configurations(1).Tools("VCLinkerTool");
		if (oLinker!=null && oLinker.LinkDll == true)
			wizard.RenderTemplate(strTemplatePath + "\\" + "root.def", strDefFileName);
		else
			wizard.RenderTemplate(strTemplatePath + "\\" + "empty.def", strDefFileName);

		//add the def file to the project
		selProj.Object.AddFile(strDefFileName);
		// add "/DEF:<filename>" to the linker options
		var nConfig;
		for (nConfig=1; nConfig<= selProj.Object.Configurations.Count; nConfig++)
		{ 
			var oLinker = selProj.Object.Configurations(nConfig).Tools("VCLinkerTool");
			if (oLinker!=null)
				oLinker.ModuleDefinitionFile = StripPath(strDefFileName);
		}

       var deffile = selProj.Object.Files( strDefFileName );
       if( deffile )
       {
           var window = deffile.Object.Open(vsViewKindPrimary);
           if(window)
               window.visible = true;
       }

		oCM.CommitTransaction();
	}
	catch(e)
	{
		if (oCM)
			oCM.AbortTransaction();

		if (e.description.length != 0)
			SetErrorInfo(e);
		return e.number
	}
}

function StripPath(strFullPath)
{
	try
	{
		var nIndex = strFullPath.lastIndexOf("\\", strFullPath.length);
		if (nIndex != -1)
		{
			return strFullPath.substring(nIndex+1, strFullPath.length);
		}
		
		return strFullPath;
	}
	catch(e)
	{
		throw e;
	}
}


// SIG // Begin signature block
// SIG // MIIjjwYJKoZIhvcNAQcCoIIjgDCCI3wCAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // +HtKD8JA6p55a0sE0RwlRJEqW7rja6YunP8AkthPnR6g
// SIG // gg2BMIIF/zCCA+egAwIBAgITMwAAAd9r8C6Sp0q00AAA
// SIG // AAAB3zANBgkqhkiG9w0BAQsFADB+MQswCQYDVQQGEwJV
// SIG // UzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMH
// SIG // UmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBv
// SIG // cmF0aW9uMSgwJgYDVQQDEx9NaWNyb3NvZnQgQ29kZSBT
// SIG // aWduaW5nIFBDQSAyMDExMB4XDTIwMTIxNTIxMzE0NVoX
// SIG // DTIxMTIwMjIxMzE0NVowdDELMAkGA1UEBhMCVVMxEzAR
// SIG // BgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1v
// SIG // bmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlv
// SIG // bjEeMBwGA1UEAxMVTWljcm9zb2Z0IENvcnBvcmF0aW9u
// SIG // MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA
// SIG // trsZWRAAo6nx5LhcqAsHy9uaHyPQ2VireMBI9yQUOPBj
// SIG // 7dVLA7/N+AnKFFDzJ7P+grT6GkOE4cv5GzjoP8yQJ6yX
// SIG // ojEKkXti7HW/zUiNoF11/ZWndf8j1Azl6OBjcD416tSW
// SIG // Yvh2VfdW1K+mY83j49YPm3qbKnfxwtV0nI9H092gMS0c
// SIG // pCUsxMRAZlPXksrjsFLqvgq4rnULVhjHSVOudL/yps3z
// SIG // OOmOpaPzAp56b898xC+zzHVHcKo/52IRht1FSC8V+7QH
// SIG // TG8+yzfuljiKU9QONa8GqDlZ7/vFGveB8IY2ZrtUu98n
// SIG // le0WWTcaIRHoCYvWGLLF2u1GVFJAggPipwIDAQABo4IB
// SIG // fjCCAXowHwYDVR0lBBgwFgYKKwYBBAGCN0wIAQYIKwYB
// SIG // BQUHAwMwHQYDVR0OBBYEFDj2zC/CHZDRrQnzJlT7byOl
// SIG // WfPjMFAGA1UdEQRJMEekRTBDMSkwJwYDVQQLEyBNaWNy
// SIG // b3NvZnQgT3BlcmF0aW9ucyBQdWVydG8gUmljbzEWMBQG
// SIG // A1UEBRMNMjMwMDEyKzQ2MzAwOTAfBgNVHSMEGDAWgBRI
// SIG // bmTlUAXTgqoXNzcitW2oynUClTBUBgNVHR8ETTBLMEmg
// SIG // R6BFhkNodHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtp
// SIG // b3BzL2NybC9NaWNDb2RTaWdQQ0EyMDExXzIwMTEtMDct
// SIG // MDguY3JsMGEGCCsGAQUFBwEBBFUwUzBRBggrBgEFBQcw
// SIG // AoZFaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3BraW9w
// SIG // cy9jZXJ0cy9NaWNDb2RTaWdQQ0EyMDExXzIwMTEtMDct
// SIG // MDguY3J0MAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQEL
// SIG // BQADggIBAJ56h7Q8mFBWlQJLwCtHqqup4aC/eUmULt0Z
// SIG // 6We7XUPPUEd/vuwPuIa6+1eMcZpAeQTm0tGCvjACxNNm
// SIG // rY8FoD3aWEOvFnSxq6CWR5G2XYBERvu7RExZd2iheCqa
// SIG // EmhjrJGV6Uz5wmjKNj16ADFTBqbEBELMIpmatyEN50UH
// SIG // wZSdD6DDHDf/j5LPGUy9QaD2LCaaJLenKpefaugsqWWC
// SIG // MIMifPdh6bbcmxyoNWbUC1JUl3HETJboD4BHDWSWoDxI
// SIG // D2J4uG9dbJ40QIH9HckNMyPWi16k8VlFOaQiBYj09G9s
// SIG // LMc0agrchqqZBjPD/RmszvHmqJlSLQmAXCUgcgcf6UtH
// SIG // EmMAQRwGcSTg1KsUl6Ehg75k36lCV57Z1pC+KJKJNRYg
// SIG // g2eI6clzkLp2+noCF75IEO429rjtujsNJvEcJXg74TjK
// SIG // 5x7LqYjj26Myq6EmuqWhbVUofPWm1EqKEfEHWXInppqB
// SIG // YXFpBMBYOLKc72DT+JyLNfd9utVsk2kTGaHHhrp+xgk9
// SIG // kZeud7lI/hfoPeHOtwIc0quJIXS+B5RSD9nj79vbJn1J
// SIG // x7RqusmBQy509Kv2Pg4t48JaBfBFpJB0bUrl5RVG05sK
// SIG // /5Qw4G6WYioS0uwgUw499iNC+Yud9vrh3M8PNqGQ5mJm
// SIG // JiFEjG2ToEuuYe/e64+SSejpHhFCaAFcMIIHejCCBWKg
// SIG // AwIBAgIKYQ6Q0gAAAAAAAzANBgkqhkiG9w0BAQsFADCB
// SIG // iDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hpbmd0
// SIG // b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1p
// SIG // Y3Jvc29mdCBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMpTWlj
// SIG // cm9zb2Z0IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9yaXR5
// SIG // IDIwMTEwHhcNMTEwNzA4MjA1OTA5WhcNMjYwNzA4MjEw
// SIG // OTA5WjB+MQswCQYDVQQGEwJVUzETMBEGA1UECBMKV2Fz
// SIG // aGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UE
// SIG // ChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSgwJgYDVQQD
// SIG // Ex9NaWNyb3NvZnQgQ29kZSBTaWduaW5nIFBDQSAyMDEx
// SIG // MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA
// SIG // q/D6chAcLq3YbqqCEE00uvK2WCGfQhsqa+laUKq4Bjga
// SIG // BEm6f8MMHt03a8YS2AvwOMKZBrDIOdUBFDFC04kNeWSH
// SIG // fpRgJGyvnkmc6Whe0t+bU7IKLMOv2akrrnoJr9eWWcpg
// SIG // GgXpZnboMlImEi/nqwhQz7NEt13YxC4Ddato88tt8zpc
// SIG // oRb0RrrgOGSsbmQ1eKagYw8t00CT+OPeBw3VXHmlSSnn
// SIG // Db6gE3e+lD3v++MrWhAfTVYoonpy4BI6t0le2O3tQ5GD
// SIG // 2Xuye4Yb2T6xjF3oiU+EGvKhL1nkkDstrjNYxbc+/jLT
// SIG // swM9sbKvkjh+0p2ALPVOVpEhNSXDOW5kf1O6nA+tGSOE
// SIG // y/S6A4aN91/w0FK/jJSHvMAhdCVfGCi2zCcoOCWYOUo2
// SIG // z3yxkq4cI6epZuxhH2rhKEmdX4jiJV3TIUs+UsS1Vz8k
// SIG // A/DRelsv1SPjcF0PUUZ3s/gA4bysAoJf28AVs70b1FVL
// SIG // 5zmhD+kjSbwYuER8ReTBw3J64HLnJN+/RpnF78IcV9uD
// SIG // jexNSTCnq47f7Fufr/zdsGbiwZeBe+3W7UvnSSmnEyim
// SIG // p31ngOaKYnhfsi+E11ecXL93KCjx7W3DKI8sj0A3T8Hh
// SIG // hUSJxAlMxdSlQy90lfdu+HggWCwTXWCVmj5PM4TasIgX
// SIG // 3p5O9JawvEagbJjS4NaIjAsCAwEAAaOCAe0wggHpMBAG
// SIG // CSsGAQQBgjcVAQQDAgEAMB0GA1UdDgQWBBRIbmTlUAXT
// SIG // gqoXNzcitW2oynUClTAZBgkrBgEEAYI3FAIEDB4KAFMA
// SIG // dQBiAEMAQTALBgNVHQ8EBAMCAYYwDwYDVR0TAQH/BAUw
// SIG // AwEB/zAfBgNVHSMEGDAWgBRyLToCMZBDuRQFTuHqp8cx
// SIG // 0SOJNDBaBgNVHR8EUzBRME+gTaBLhklodHRwOi8vY3Js
// SIG // Lm1pY3Jvc29mdC5jb20vcGtpL2NybC9wcm9kdWN0cy9N
// SIG // aWNSb29DZXJBdXQyMDExXzIwMTFfMDNfMjIuY3JsMF4G
// SIG // CCsGAQUFBwEBBFIwUDBOBggrBgEFBQcwAoZCaHR0cDov
// SIG // L3d3dy5taWNyb3NvZnQuY29tL3BraS9jZXJ0cy9NaWNS
// SIG // b29DZXJBdXQyMDExXzIwMTFfMDNfMjIuY3J0MIGfBgNV
// SIG // HSAEgZcwgZQwgZEGCSsGAQQBgjcuAzCBgzA/BggrBgEF
// SIG // BQcCARYzaHR0cDovL3d3dy5taWNyb3NvZnQuY29tL3Br
// SIG // aW9wcy9kb2NzL3ByaW1hcnljcHMuaHRtMEAGCCsGAQUF
// SIG // BwICMDQeMiAdAEwAZQBnAGEAbABfAHAAbwBsAGkAYwB5
// SIG // AF8AcwB0AGEAdABlAG0AZQBuAHQALiAdMA0GCSqGSIb3
// SIG // DQEBCwUAA4ICAQBn8oalmOBUeRou09h0ZyKbC5YR4WOS
// SIG // mUKWfdJ5DJDBZV8uLD74w3LRbYP+vj/oCso7v0epo/Np
// SIG // 22O/IjWll11lhJB9i0ZQVdgMknzSGksc8zxCi1LQsP1r
// SIG // 4z4HLimb5j0bpdS1HXeUOeLpZMlEPXh6I/MTfaaQdION
// SIG // 9MsmAkYqwooQu6SpBQyb7Wj6aC6VoCo/KmtYSWMfCWlu
// SIG // WpiW5IP0wI/zRive/DvQvTXvbiWu5a8n7dDd8w6vmSiX
// SIG // mE0OPQvyCInWH8MyGOLwxS3OW560STkKxgrCxq2u5bLZ
// SIG // 2xWIUUVYODJxJxp/sfQn+N4sOiBpmLJZiWhub6e3dMNA
// SIG // BQamASooPoI/E01mC8CzTfXhj38cbxV9Rad25UAqZaPD
// SIG // XVJihsMdYzaXht/a8/jyFqGaJ+HNpZfQ7l1jQeNbB5yH
// SIG // PgZ3BtEGsXUfFL5hYbXw3MYbBL7fQccOKO7eZS/sl/ah
// SIG // XJbYANahRr1Z85elCUtIEJmAH9AAKcWxm6U/RXceNcbS
// SIG // oqKfenoi+kiVH6v7RyOA9Z74v2u3S5fi63V4GuzqN5l5
// SIG // GEv/1rMjaHXmr/r8i+sLgOppO6/8MO0ETI7f33VtY5E9
// SIG // 0Z1WTk+/gFcioXgRMiF670EKsT/7qMykXcGhiJtXcVZO
// SIG // SEXAQsmbdlsKgEhr/Xmfwb1tbWrJUnMTDXpQzTGCFWYw
// SIG // ghViAgEBMIGVMH4xCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAm
// SIG // BgNVBAMTH01pY3Jvc29mdCBDb2RlIFNpZ25pbmcgUENB
// SIG // IDIwMTECEzMAAAHfa/AukqdKtNAAAAAAAd8wDQYJYIZI
// SIG // AWUDBAIBBQCgga4wGQYJKoZIhvcNAQkDMQwGCisGAQQB
// SIG // gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcC
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEIEO3e0Ttw6vintdJVxzC
// SIG // UOR40lOj1bku0FuOn7iyK8Y7MEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEAsUcDyA63hC0eUR8ZrH3wzttaOrgjeJsAjXqD
// SIG // z3sq645EZpqPtTh2z+Rrqi5zS/OfLrOEqKnGrylxCSgN
// SIG // DvHz8rkJIm2OUUeOLabgAGd2oVncP5lWfdT4oHApXvMp
// SIG // hP0SXnY4QoF8CoCkP2Wvn40ujjmIlmFz0g89MtgXM9fv
// SIG // fSiVipJnkt1qODLo7rOQirSchS3rCFTFvqVLXsF5dKRh
// SIG // V9XvfQsvNLfUB6lYFssMYZF8FbxLGM5AOZN9acV7xm1p
// SIG // 2G3MHgX3rfv6Hw6Rzkw0vXrBpIiWbw5Ug4N7uJFp6DQy
// SIG // LpRz6cWT3g+To/Nt34Kj0YqFc7wKGeEjH4WRmMQGoqGC
// SIG // EvAwghLsBgorBgEEAYI3AwMBMYIS3DCCEtgGCSqGSIb3
// SIG // DQEHAqCCEskwghLFAgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFUBgsqhkiG9w0BCRABBKCCAUMEggE/MIIBOwIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCBnYRFK
// SIG // IXVgAOkiT+wp6sr6uje4IHpb4Qmx5ylp8vdQmwIGYIn5
// SIG // aQDAGBIyMDIxMDUwNjA0MzQwNC4yN1owBIACAfSggdSk
// SIG // gdEwgc4xCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
// SIG // aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
// SIG // ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKTAnBgNVBAsT
// SIG // IE1pY3Jvc29mdCBPcGVyYXRpb25zIFB1ZXJ0byBSaWNv
// SIG // MSYwJAYDVQQLEx1UaGFsZXMgVFNTIEVTTjo0NjJGLUUz
// SIG // MTktM0YyMDElMCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUt
// SIG // U3RhbXAgU2VydmljZaCCDkQwggT1MIID3aADAgECAhMz
// SIG // AAABWHBaIve+luYDAAAAAAFYMA0GCSqGSIb3DQEBCwUA
// SIG // MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5n
// SIG // dG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVN
// SIG // aWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMTHU1p
// SIG // Y3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMB4XDTIx
// SIG // MDExNDE5MDIxNFoXDTIyMDQxMTE5MDIxNFowgc4xCzAJ
// SIG // BgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
// SIG // DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3Nv
// SIG // ZnQgQ29ycG9yYXRpb24xKTAnBgNVBAsTIE1pY3Jvc29m
// SIG // dCBPcGVyYXRpb25zIFB1ZXJ0byBSaWNvMSYwJAYDVQQL
// SIG // Ex1UaGFsZXMgVFNTIEVTTjo0NjJGLUUzMTktM0YyMDEl
// SIG // MCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAgU2Vy
// SIG // dmljZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoC
// SIG // ggEBAKEfC5dg9auw0KAFGwv17yMFG8SfqgUUFC8Dzwa8
// SIG // mrps0eyhRQ2Nv9K7/sz/fVE1o/1fZp4q4SGitcsjPGtO
// SIG // njWx45VIFTINQpdoOhmsPdnFy3gBXpMGtTfXqLnnUE4+
// SIG // VmKC2vAhOZ06U6vt5Cc0cJoqEJtzOWRwEaz8BoX2nCX1
// SIG // RBXkH3PiAu7tWJv3V8zhRSPLFmeiJ+CIway04AUgmrwX
// SIG // EQHvJHgb6PiLCxgE2VABCDNT5CVyieNapcZiKx16QbDl
// SIG // e7KOwkjMEIKkcxR+32dDMtzCtpIUDgrKxmjx+Gm94jHi
// SIG // eohOHUuhl3u3hlAYfv2SA/86T1UNAiBQg3Wu9xsCAwEA
// SIG // AaOCARswggEXMB0GA1UdDgQWBBRLcNkbfZ0cGB/u536g
// SIG // e5Mn06L5WDAfBgNVHSMEGDAWgBTVYzpcijGQ80N7fEYb
// SIG // xTNoWoVtVTBWBgNVHR8ETzBNMEugSaBHhkVodHRwOi8v
// SIG // Y3JsLm1pY3Jvc29mdC5jb20vcGtpL2NybC9wcm9kdWN0
// SIG // cy9NaWNUaW1TdGFQQ0FfMjAxMC0wNy0wMS5jcmwwWgYI
// SIG // KwYBBQUHAQEETjBMMEoGCCsGAQUFBzAChj5odHRwOi8v
// SIG // d3d3Lm1pY3Jvc29mdC5jb20vcGtpL2NlcnRzL01pY1Rp
// SIG // bVN0YVBDQV8yMDEwLTA3LTAxLmNydDAMBgNVHRMBAf8E
// SIG // AjAAMBMGA1UdJQQMMAoGCCsGAQUFBwMIMA0GCSqGSIb3
// SIG // DQEBCwUAA4IBAQA53ygDWovQrh8fuliNXW0CUBTzfA4S
// SIG // l4h+IPEh5lNdrhDFy6T4MA9jup1zzlFkpYrUc0sTfQCA
// SIG // OnAjmunPgnmaS5bSf2VH8Mg34U2qgPLInMAkGaBs/Bza
// SIG // bJ65YKe1P5IKZN7Wj2bRfCK03ES8kS7g6YQH67ixMCQC
// SIG // LDreWDKJYsNs0chNpJOAzyJeGfyRUe+TUUbFwjsC/18K
// SIG // mYODVgpRSYZx0W7jrGqlJVEehuwpSIsGOYCBMnJDNdKn
// SIG // P+13Cg68cVtCNX6kJdvUFH0ZiuPMlBYD7GrCPqARlSn+
// SIG // vxffMivu2DMJJLkeywxSfD52sDV+NBf5IniuKFcE9y0m
// SIG // 9m2jMIIGcTCCBFmgAwIBAgIKYQmBKgAAAAAAAjANBgkq
// SIG // hkiG9w0BAQsFADCBiDELMAkGA1UEBhMCVVMxEzARBgNV
// SIG // BAgTCldhc2hpbmd0b24xEDAOBgNVBAcTB1JlZG1vbmQx
// SIG // HjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEy
// SIG // MDAGA1UEAxMpTWljcm9zb2Z0IFJvb3QgQ2VydGlmaWNh
// SIG // dGUgQXV0aG9yaXR5IDIwMTAwHhcNMTAwNzAxMjEzNjU1
// SIG // WhcNMjUwNzAxMjE0NjU1WjB8MQswCQYDVQQGEwJVUzET
// SIG // MBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVk
// SIG // bW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0
// SIG // aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFt
// SIG // cCBQQ0EgMjAxMDCCASIwDQYJKoZIhvcNAQEBBQADggEP
// SIG // ADCCAQoCggEBAKkdDbx3EYo6IOz8E5f1+n9plGt0VBDV
// SIG // pQoAgoX77XxoSyxfxcPlYcJ2tz5mK1vwFVMnBDEfQRsa
// SIG // lR3OCROOfGEwWbEwRA/xYIiEVEMM1024OAizQt2TrNZz
// SIG // MFcmgqNFDdDq9UeBzb8kYDJYYEbyWEeGMoQedGFnkV+B
// SIG // VLHPk0ySwcSmXdFhE24oxhr5hoC732H8RsEnHSRnEnIa
// SIG // IYqvS2SJUGKxXf13Hz3wV3WsvYpCTUBR0Q+cBj5nf/Vm
// SIG // wAOWRH7v0Ev9buWayrGo8noqCjHw2k4GkbaICDXoeByw
// SIG // 6ZnNPOcvRLqn9NxkvaQBwSAJk3jN/LzAyURdXhacAQVP
// SIG // Ik0CAwEAAaOCAeYwggHiMBAGCSsGAQQBgjcVAQQDAgEA
// SIG // MB0GA1UdDgQWBBTVYzpcijGQ80N7fEYbxTNoWoVtVTAZ
// SIG // BgkrBgEEAYI3FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8E
// SIG // BAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAfBgNVHSMEGDAW
// SIG // gBTV9lbLj+iiXGJo0T2UkFvXzpoYxDBWBgNVHR8ETzBN
// SIG // MEugSaBHhkVodHRwOi8vY3JsLm1pY3Jvc29mdC5jb20v
// SIG // cGtpL2NybC9wcm9kdWN0cy9NaWNSb29DZXJBdXRfMjAx
// SIG // MC0wNi0yMy5jcmwwWgYIKwYBBQUHAQEETjBMMEoGCCsG
// SIG // AQUFBzAChj5odHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20v
// SIG // cGtpL2NlcnRzL01pY1Jvb0NlckF1dF8yMDEwLTA2LTIz
// SIG // LmNydDCBoAYDVR0gAQH/BIGVMIGSMIGPBgkrBgEEAYI3
// SIG // LgMwgYEwPQYIKwYBBQUHAgEWMWh0dHA6Ly93d3cubWlj
// SIG // cm9zb2Z0LmNvbS9QS0kvZG9jcy9DUFMvZGVmYXVsdC5o
// SIG // dG0wQAYIKwYBBQUHAgIwNB4yIB0ATABlAGcAYQBsAF8A
// SIG // UABvAGwAaQBjAHkAXwBTAHQAYQB0AGUAbQBlAG4AdAAu
// SIG // IB0wDQYJKoZIhvcNAQELBQADggIBAAfmiFEN4sbgmD+B
// SIG // cQM9naOhIW+z66bM9TG+zwXiqf76V20ZMLPCxWbJat/1
// SIG // 5/B4vceoniXj+bzta1RXCCtRgkQS+7lTjMz0YBKKdsxA
// SIG // QEGb3FwX/1z5Xhc1mCRWS3TvQhDIr79/xn/yN31aPxzy
// SIG // mXlKkVIArzgPF/UveYFl2am1a+THzvbKegBvSzBEJCI8
// SIG // z+0DpZaPWSm8tv0E4XCfMkon/VWvL/625Y4zu2JfmttX
// SIG // QOnxzplmkIz/amJ/3cVKC5Em4jnsGUpxY517IW3DnKOi
// SIG // PPp/fZZqkHimbdLhnPkd/DjYlPTGpQqWhqS9nhquBEKD
// SIG // uLWAmyI4ILUl5WTs9/S/fmNZJQ96LjlXdqJxqgaKD4kW
// SIG // umGnEcua2A5HmoDF0M2n0O99g/DhO3EJ3110mCIIYdqw
// SIG // UB5vvfHhAN/nMQekkzr3ZUd46PioSKv33nJ+YWtvd6mB
// SIG // y6cJrDm77MbL2IK0cs0d9LiFAR6A+xuJKlQ5slvayA1V
// SIG // mXqHczsI5pgt6o3gMy4SKfXAL1QnIffIrE7aKLixqduW
// SIG // sqdCosnPGUFN4Ib5KpqjEWYw07t0MkvfY3v1mYovG8ch
// SIG // r1m1rtxEPJdQcdeh0sVV42neV8HR3jDA/czmTfsNv11P
// SIG // 6Z0eGTgvvM9YBS7vDaBQNdrvCScc1bN+NR4Iuto229Nf
// SIG // j950iEkSoYIC0jCCAjsCAQEwgfyhgdSkgdEwgc4xCzAJ
// SIG // BgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAw
// SIG // DgYDVQQHEwdSZWRtb25kMR4wHAYDVQQKExVNaWNyb3Nv
// SIG // ZnQgQ29ycG9yYXRpb24xKTAnBgNVBAsTIE1pY3Jvc29m
// SIG // dCBPcGVyYXRpb25zIFB1ZXJ0byBSaWNvMSYwJAYDVQQL
// SIG // Ex1UaGFsZXMgVFNTIEVTTjo0NjJGLUUzMTktM0YyMDEl
// SIG // MCMGA1UEAxMcTWljcm9zb2Z0IFRpbWUtU3RhbXAgU2Vy
// SIG // dmljZaIjCgEBMAcGBSsOAwIaAxUAqckrcxrn0Qshpuoz
// SIG // jp+l+DSfNL+ggYMwgYCkfjB8MQswCQYDVQQGEwJVUzET
// SIG // MBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVk
// SIG // bW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENvcnBvcmF0
// SIG // aW9uMSYwJAYDVQQDEx1NaWNyb3NvZnQgVGltZS1TdGFt
// SIG // cCBQQ0EgMjAxMDANBgkqhkiG9w0BAQUFAAIFAOQ9sisw
// SIG // IhgPMjAyMTA1MDYwNDA5MTVaGA8yMDIxMDUwNzA0MDkx
// SIG // NVowdzA9BgorBgEEAYRZCgQBMS8wLTAKAgUA5D2yKwIB
// SIG // ADAKAgEAAgIUHwIB/zAHAgEAAgIREjAKAgUA5D8DqwIB
// SIG // ADA2BgorBgEEAYRZCgQCMSgwJjAMBgorBgEEAYRZCgMC
// SIG // oAowCAIBAAIDB6EgoQowCAIBAAIDAYagMA0GCSqGSIb3
// SIG // DQEBBQUAA4GBAAXNkZGupPef1PNIKnTe2hjtYCPgsjs7
// SIG // HZlkkacI0WIM0IuO3roRDEeNna6up2Qv9I32RdwuKDav
// SIG // SnqKGGDtMPk8KdkEG0jL5eF93RXRrnALRiCEZp/oMYan
// SIG // RmD6XJvUVYyDXFr/hPej/yUaIIBPsUVOkRf8hAxtyr6J
// SIG // /xzPele7MYIDDTCCAwkCAQEwgZMwfDELMAkGA1UEBhMC
// SIG // VVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNVBAcT
// SIG // B1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBDb3Jw
// SIG // b3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRpbWUt
// SIG // U3RhbXAgUENBIDIwMTACEzMAAAFYcFoi976W5gMAAAAA
// SIG // AVgwDQYJYIZIAWUDBAIBBQCgggFKMBoGCSqGSIb3DQEJ
// SIG // AzENBgsqhkiG9w0BCRABBDAvBgkqhkiG9w0BCQQxIgQg
// SIG // B4L7OE25O2dpg3Gh8N/62WeG1fXWVSHAp59nIJxstrEw
// SIG // gfoGCyqGSIb3DQEJEAIvMYHqMIHnMIHkMIG9BCDySjON
// SIG // bIY1l2zKT4ba4sCI4WkBC6sIfR9uSVNVx3DTBzCBmDCB
// SIG // gKR+MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
// SIG // aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
// SIG // ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMT
// SIG // HU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwAhMz
// SIG // AAABWHBaIve+luYDAAAAAAFYMCIEIDrkFXjJGwtdIvFr
// SIG // Wwu2Im09/jpifdP5IpPu6HOadnZ+MA0GCSqGSIb3DQEB
// SIG // CwUABIIBACUJGAzAet3uRSAqMmOeMpYm1eslI1s9rJ7U
// SIG // sdXge2pVgpm0QFwqvORp975akMg0ciARsD/0WGSHQ6mf
// SIG // Cf72CpdiaDfYvEegBhatJLq9I33KxVC/cCqUSvKpMm8/
// SIG // 69WjLkcyCaqZv07hOY5QMm8qNAU/UgYVnZ7BcEQ6FPYa
// SIG // nIuxi/YzGyjdcRSrHZ4WLlXSPY12aSjj5L2msLLV13ie
// SIG // 7bWaZoHeX5ZnNwHoiqWPf+9xGWtwbW3QcSQ7OQq2wrI+
// SIG // kqxd200v3c9X8v2eSmzEFDV1pdgMYx4/PYQHk55sdv6Y
// SIG // qldHJcbdTZ3tt2sXOAru3PoFemjqkEiLI++y/MVv7O8=
// SIG // End signature block
