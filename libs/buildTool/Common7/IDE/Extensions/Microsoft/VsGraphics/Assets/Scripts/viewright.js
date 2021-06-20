﻿
var coord = document.getCoordinateSystemMatrix();
var relPos = math.getRightVector(coord);
relPos = math.scaleVector(relPos, -1);

document.frameSelection(relPos[0], relPos[1], relPos[2], true);

var currentCamera = document.getCurrentCamera();
if (currentCamera.typeId == "Microsoft.VisualStudio.3D.OrthographicCamera")
{
    var GridPlaneOrientationTopFront = 1;
    document.setGridOrientation(GridPlaneOrientationTopFront);
}
// SIG // Begin signature block
// SIG // MIIjggYJKoZIhvcNAQcCoIIjczCCI28CAQExDzANBglg
// SIG // hkgBZQMEAgEFADB3BgorBgEEAYI3AgEEoGkwZzAyBgor
// SIG // BgEEAYI3AgEeMCQCAQEEEBDgyQbOONQRoqMAEEvTUJAC
// SIG // AQACAQACAQACAQACAQAwMTANBglghkgBZQMEAgEFAAQg
// SIG // wiMNTx2G+OA8hlxmUwr6A5sIk/nKnhvwb9Luax11wfOg
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
// SIG // SEXAQsmbdlsKgEhr/Xmfwb1tbWrJUnMTDXpQzTGCFVkw
// SIG // ghVVAgEBMIGVMH4xCzAJBgNVBAYTAlVTMRMwEQYDVQQI
// SIG // EwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4w
// SIG // HAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xKDAm
// SIG // BgNVBAMTH01pY3Jvc29mdCBDb2RlIFNpZ25pbmcgUENB
// SIG // IDIwMTECEzMAAAHfa/AukqdKtNAAAAAAAd8wDQYJYIZI
// SIG // AWUDBAIBBQCgga4wGQYJKoZIhvcNAQkDMQwGCisGAQQB
// SIG // gjcCAQQwHAYKKwYBBAGCNwIBCzEOMAwGCisGAQQBgjcC
// SIG // ARUwLwYJKoZIhvcNAQkEMSIEINQPagOGh2i3e+9TkgOs
// SIG // XZ2XAuHbgbScX+3fceEg7PHUMEIGCisGAQQBgjcCAQwx
// SIG // NDAyoBSAEgBNAGkAYwByAG8AcwBvAGYAdKEagBhodHRw
// SIG // Oi8vd3d3Lm1pY3Jvc29mdC5jb20wDQYJKoZIhvcNAQEB
// SIG // BQAEggEAe34xNu+8rhcDC3m6buqjMCGA4NLADQnMTQtm
// SIG // RYrvqq4uhQFKzimeBbj4Yo7xvBklKID00+9TA8Q1tPKQ
// SIG // 7E+2hEiMt8+gZ+Bkl3VEvweXR03jNHS1jRS4IjH09aVp
// SIG // FTDMcJRuadbt9+xt2TVBXxHY4hJSpZy+y0gqJFeM1KiA
// SIG // XFgRE3QdFcioBV9fg1Riub7QPI3UdVPRGZgUnk6eOioA
// SIG // fkKW9+25FcVPTxKxLms5LlRYFkokQ68+uNi8PCLm7QsP
// SIG // okftgNSrD9CK7v16otNfYpU4f+aWPPNAwxduxqdjReKH
// SIG // z8Xzg7pNGz4tBLVYo+tcDy2E2xOfW5pxIrWwbn6Yl6GC
// SIG // EuMwghLfBgorBgEEAYI3AwMBMYISzzCCEssGCSqGSIb3
// SIG // DQEHAqCCErwwghK4AgEDMQ8wDQYJYIZIAWUDBAIBBQAw
// SIG // ggFPBgsqhkiG9w0BCRABBKCCAT4EggE6MIIBNgIBAQYK
// SIG // KwYBBAGEWQoDATAxMA0GCWCGSAFlAwQCAQUABCDJIMnR
// SIG // fvRKOFJQL076uqSS6iROTXBNGyEQbYM7JFoCNgIGYK5Z
// SIG // XO/ZGBEyMDIxMDYxMTAwMjUyNC45WjAEgAIB9KCB0KSB
// SIG // zTCByjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
// SIG // bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
// SIG // FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjElMCMGA1UECxMc
// SIG // TWljcm9zb2Z0IEFtZXJpY2EgT3BlcmF0aW9uczEmMCQG
// SIG // A1UECxMdVGhhbGVzIFRTUyBFU046QUUyQy1FMzJCLTFB
// SIG // RkMxJTAjBgNVBAMTHE1pY3Jvc29mdCBUaW1lLVN0YW1w
// SIG // IFNlcnZpY2Wggg48MIIE8TCCA9mgAwIBAgITMwAAAUii
// SIG // iEVWvC+AvwAAAAABSDANBgkqhkiG9w0BAQsFADB8MQsw
// SIG // CQYDVQQGEwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQ
// SIG // MA4GA1UEBxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9z
// SIG // b2Z0IENvcnBvcmF0aW9uMSYwJAYDVQQDEx1NaWNyb3Nv
// SIG // ZnQgVGltZS1TdGFtcCBQQ0EgMjAxMDAeFw0yMDExMTIx
// SIG // ODI1NTZaFw0yMjAyMTExODI1NTZaMIHKMQswCQYDVQQG
// SIG // EwJVUzETMBEGA1UECBMKV2FzaGluZ3RvbjEQMA4GA1UE
// SIG // BxMHUmVkbW9uZDEeMBwGA1UEChMVTWljcm9zb2Z0IENv
// SIG // cnBvcmF0aW9uMSUwIwYDVQQLExxNaWNyb3NvZnQgQW1l
// SIG // cmljYSBPcGVyYXRpb25zMSYwJAYDVQQLEx1UaGFsZXMg
// SIG // VFNTIEVTTjpBRTJDLUUzMkItMUFGQzElMCMGA1UEAxMc
// SIG // TWljcm9zb2Z0IFRpbWUtU3RhbXAgU2VydmljZTCCASIw
// SIG // DQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAPf/eK8V
// SIG // hIrR0a1xdpnyk0sQBFutol7CvKL8mBtoqWruGEUhTUSR
// SIG // jQcyIVGbb9ay6S3raTbnO8PFbXp+mGDnzvr6XXBicKWB
// SIG // 7+sWWtHZS7VbC0WZzXEanLj67GhsIQx4gUMaEvkM1Nts
// SIG // 6KwusFRhvJevt8/PFtWQTAkM/kUbafSujWo6N6AiQsiN
// SIG // dTqpqF25DFN+w84SnyOCowqf75+kfp+9cqV7BbN2x++G
// SIG // JngfgirUJM6f5+TGpwPBMdiRhliCqb7VfXHvjnyunr2q
// SIG // MV5U/cZsC8ltjdsWWrm7LGuI9xnENmsHp/XAAtes7b8h
// SIG // 19UR3BNaokgNkEmNPJxCGHFLeLUCAwEAAaOCARswggEX
// SIG // MB0GA1UdDgQWBBSHMvBpnw4EtEkfSz0TrekhxYmiRzAf
// SIG // BgNVHSMEGDAWgBTVYzpcijGQ80N7fEYbxTNoWoVtVTBW
// SIG // BgNVHR8ETzBNMEugSaBHhkVodHRwOi8vY3JsLm1pY3Jv
// SIG // c29mdC5jb20vcGtpL2NybC9wcm9kdWN0cy9NaWNUaW1T
// SIG // dGFQQ0FfMjAxMC0wNy0wMS5jcmwwWgYIKwYBBQUHAQEE
// SIG // TjBMMEoGCCsGAQUFBzAChj5odHRwOi8vd3d3Lm1pY3Jv
// SIG // c29mdC5jb20vcGtpL2NlcnRzL01pY1RpbVN0YVBDQV8y
// SIG // MDEwLTA3LTAxLmNydDAMBgNVHRMBAf8EAjAAMBMGA1Ud
// SIG // JQQMMAoGCCsGAQUFBwMIMA0GCSqGSIb3DQEBCwUAA4IB
// SIG // AQBmEpbBsycL1DLByuWCzSpf1uHGJka977wkQPbuE6LI
// SIG // ZrxH2erMeDj6ro0p9hd9Lra4xQmna74nu0g1RN//W6Av
// SIG // 4rhHCw9V6ENqxIkmP7tugjFk/wBT/FB1VjuqCZAaZ3gX
// SIG // 5zmGQkm2Xo1F5gT3TxDr3yqPWYOmvQzH7guE/+1Oovoe
// SIG // lkRRGWEU512fItK1UIV180jXIYc/2fTI3Up+EYRXezsw
// SIG // HOkUlfA5+XCDIT1zBDyOM5NWk1F0Kov8/lWIICjZ1yIe
// SIG // N3W7WLVDhBnrFvT4HUd1kVEQs6IXB/SyHdnXE55IVweD
// SIG // SssT6ux4fPhCG+gSPuH1sPxU3v8ecIPBakElMIIGcTCC
// SIG // BFmgAwIBAgIKYQmBKgAAAAAAAjANBgkqhkiG9w0BAQsF
// SIG // ADCBiDELMAkGA1UEBhMCVVMxEzARBgNVBAgTCldhc2hp
// SIG // bmd0b24xEDAOBgNVBAcTB1JlZG1vbmQxHjAcBgNVBAoT
// SIG // FU1pY3Jvc29mdCBDb3Jwb3JhdGlvbjEyMDAGA1UEAxMp
// SIG // TWljcm9zb2Z0IFJvb3QgQ2VydGlmaWNhdGUgQXV0aG9y
// SIG // aXR5IDIwMTAwHhcNMTAwNzAxMjEzNjU1WhcNMjUwNzAx
// SIG // MjE0NjU1WjB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMK
// SIG // V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
// SIG // A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYD
// SIG // VQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAx
// SIG // MDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB
// SIG // AKkdDbx3EYo6IOz8E5f1+n9plGt0VBDVpQoAgoX77Xxo
// SIG // SyxfxcPlYcJ2tz5mK1vwFVMnBDEfQRsalR3OCROOfGEw
// SIG // WbEwRA/xYIiEVEMM1024OAizQt2TrNZzMFcmgqNFDdDq
// SIG // 9UeBzb8kYDJYYEbyWEeGMoQedGFnkV+BVLHPk0ySwcSm
// SIG // XdFhE24oxhr5hoC732H8RsEnHSRnEnIaIYqvS2SJUGKx
// SIG // Xf13Hz3wV3WsvYpCTUBR0Q+cBj5nf/VmwAOWRH7v0Ev9
// SIG // buWayrGo8noqCjHw2k4GkbaICDXoeByw6ZnNPOcvRLqn
// SIG // 9NxkvaQBwSAJk3jN/LzAyURdXhacAQVPIk0CAwEAAaOC
// SIG // AeYwggHiMBAGCSsGAQQBgjcVAQQDAgEAMB0GA1UdDgQW
// SIG // BBTVYzpcijGQ80N7fEYbxTNoWoVtVTAZBgkrBgEEAYI3
// SIG // FAIEDB4KAFMAdQBiAEMAQTALBgNVHQ8EBAMCAYYwDwYD
// SIG // VR0TAQH/BAUwAwEB/zAfBgNVHSMEGDAWgBTV9lbLj+ii
// SIG // XGJo0T2UkFvXzpoYxDBWBgNVHR8ETzBNMEugSaBHhkVo
// SIG // dHRwOi8vY3JsLm1pY3Jvc29mdC5jb20vcGtpL2NybC9w
// SIG // cm9kdWN0cy9NaWNSb29DZXJBdXRfMjAxMC0wNi0yMy5j
// SIG // cmwwWgYIKwYBBQUHAQEETjBMMEoGCCsGAQUFBzAChj5o
// SIG // dHRwOi8vd3d3Lm1pY3Jvc29mdC5jb20vcGtpL2NlcnRz
// SIG // L01pY1Jvb0NlckF1dF8yMDEwLTA2LTIzLmNydDCBoAYD
// SIG // VR0gAQH/BIGVMIGSMIGPBgkrBgEEAYI3LgMwgYEwPQYI
// SIG // KwYBBQUHAgEWMWh0dHA6Ly93d3cubWljcm9zb2Z0LmNv
// SIG // bS9QS0kvZG9jcy9DUFMvZGVmYXVsdC5odG0wQAYIKwYB
// SIG // BQUHAgIwNB4yIB0ATABlAGcAYQBsAF8AUABvAGwAaQBj
// SIG // AHkAXwBTAHQAYQB0AGUAbQBlAG4AdAAuIB0wDQYJKoZI
// SIG // hvcNAQELBQADggIBAAfmiFEN4sbgmD+BcQM9naOhIW+z
// SIG // 66bM9TG+zwXiqf76V20ZMLPCxWbJat/15/B4vceoniXj
// SIG // +bzta1RXCCtRgkQS+7lTjMz0YBKKdsxAQEGb3FwX/1z5
// SIG // Xhc1mCRWS3TvQhDIr79/xn/yN31aPxzymXlKkVIArzgP
// SIG // F/UveYFl2am1a+THzvbKegBvSzBEJCI8z+0DpZaPWSm8
// SIG // tv0E4XCfMkon/VWvL/625Y4zu2JfmttXQOnxzplmkIz/
// SIG // amJ/3cVKC5Em4jnsGUpxY517IW3DnKOiPPp/fZZqkHim
// SIG // bdLhnPkd/DjYlPTGpQqWhqS9nhquBEKDuLWAmyI4ILUl
// SIG // 5WTs9/S/fmNZJQ96LjlXdqJxqgaKD4kWumGnEcua2A5H
// SIG // moDF0M2n0O99g/DhO3EJ3110mCIIYdqwUB5vvfHhAN/n
// SIG // MQekkzr3ZUd46PioSKv33nJ+YWtvd6mBy6cJrDm77MbL
// SIG // 2IK0cs0d9LiFAR6A+xuJKlQ5slvayA1VmXqHczsI5pgt
// SIG // 6o3gMy4SKfXAL1QnIffIrE7aKLixqduWsqdCosnPGUFN
// SIG // 4Ib5KpqjEWYw07t0MkvfY3v1mYovG8chr1m1rtxEPJdQ
// SIG // cdeh0sVV42neV8HR3jDA/czmTfsNv11P6Z0eGTgvvM9Y
// SIG // BS7vDaBQNdrvCScc1bN+NR4Iuto229Nfj950iEkSoYIC
// SIG // zjCCAjcCAQEwgfihgdCkgc0wgcoxCzAJBgNVBAYTAlVT
// SIG // MRMwEQYDVQQIEwpXYXNoaW5ndG9uMRAwDgYDVQQHEwdS
// SIG // ZWRtb25kMR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9y
// SIG // YXRpb24xJTAjBgNVBAsTHE1pY3Jvc29mdCBBbWVyaWNh
// SIG // IE9wZXJhdGlvbnMxJjAkBgNVBAsTHVRoYWxlcyBUU1Mg
// SIG // RVNOOkFFMkMtRTMyQi0xQUZDMSUwIwYDVQQDExxNaWNy
// SIG // b3NvZnQgVGltZS1TdGFtcCBTZXJ2aWNloiMKAQEwBwYF
// SIG // Kw4DAhoDFQCHK4KWuhxZ/hIhxFp8ARfVGGzrOaCBgzCB
// SIG // gKR+MHwxCzAJBgNVBAYTAlVTMRMwEQYDVQQIEwpXYXNo
// SIG // aW5ndG9uMRAwDgYDVQQHEwdSZWRtb25kMR4wHAYDVQQK
// SIG // ExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xJjAkBgNVBAMT
// SIG // HU1pY3Jvc29mdCBUaW1lLVN0YW1wIFBDQSAyMDEwMA0G
// SIG // CSqGSIb3DQEBBQUAAgUA5Gyd9DAiGA8yMDIxMDYxMDIy
// SIG // MTkzMloYDzIwMjEwNjExMjIxOTMyWjB3MD0GCisGAQQB
// SIG // hFkKBAExLzAtMAoCBQDkbJ30AgEAMAoCAQACAgKFAgH/
// SIG // MAcCAQACAhGdMAoCBQDkbe90AgEAMDYGCisGAQQBhFkK
// SIG // BAIxKDAmMAwGCisGAQQBhFkKAwKgCjAIAgEAAgMHoSCh
// SIG // CjAIAgEAAgMBhqAwDQYJKoZIhvcNAQEFBQADgYEAA2zh
// SIG // xpyxm1+GI1VM2TngnvsyAB6JNA5ZsFoZyttrkeB9hH+I
// SIG // yMeoA4ZisfTcyxpNKfnQ3MEsjjgvPvTnP0gRLoWak2l/
// SIG // +Oe37we/Bdnz5X0okodUmh11sgAerwTtcqZHAz9cMYG0
// SIG // vR3CbC7aSyTHsBTvzrr8CPJcjJMopae6PDcxggMNMIID
// SIG // CQIBATCBkzB8MQswCQYDVQQGEwJVUzETMBEGA1UECBMK
// SIG // V2FzaGluZ3RvbjEQMA4GA1UEBxMHUmVkbW9uZDEeMBwG
// SIG // A1UEChMVTWljcm9zb2Z0IENvcnBvcmF0aW9uMSYwJAYD
// SIG // VQQDEx1NaWNyb3NvZnQgVGltZS1TdGFtcCBQQ0EgMjAx
// SIG // MAITMwAAAUiiiEVWvC+AvwAAAAABSDANBglghkgBZQME
// SIG // AgEFAKCCAUowGgYJKoZIhvcNAQkDMQ0GCyqGSIb3DQEJ
// SIG // EAEEMC8GCSqGSIb3DQEJBDEiBCCBm4V5xSFrB+7TbpRx
// SIG // IJxHROZ41KZCk3e0RYBgUjt5ezCB+gYLKoZIhvcNAQkQ
// SIG // Ai8xgeowgecwgeQwgb0EIKmQGuqMeaG/Jh/m1NxO8Plj
// SIG // hr5Xv1PBVXpPVoDB22jYMIGYMIGApH4wfDELMAkGA1UE
// SIG // BhMCVVMxEzARBgNVBAgTCldhc2hpbmd0b24xEDAOBgNV
// SIG // BAcTB1JlZG1vbmQxHjAcBgNVBAoTFU1pY3Jvc29mdCBD
// SIG // b3Jwb3JhdGlvbjEmMCQGA1UEAxMdTWljcm9zb2Z0IFRp
// SIG // bWUtU3RhbXAgUENBIDIwMTACEzMAAAFIoohFVrwvgL8A
// SIG // AAAAAUgwIgQgv7PLEx3SW0hm84MXrj5D7SD7h4ASoi74
// SIG // ZVlkFfkUUa4wDQYJKoZIhvcNAQELBQAEggEAfc4OOzPH
// SIG // p+SqkyfeiTVGw/PXycvCN6zKQ5rK3xXN8Am8jDf1gf6Z
// SIG // Z9IzjmQynPIZlSiDggnrWXYYdIlDJUDEfyCJOu/QfZZP
// SIG // rMl2LEVly5Ce0ibef8CrxZu3vCgpa/Z3wB6o9gicCBHt
// SIG // t9Xm+F+NRSfZmucX1UhrTqX3O2bs36JAEbJ9r7/PVc4p
// SIG // T+zdgylRRAqPadAhoqyASqVqFJp3mjlR4Hr6Hz8YpICU
// SIG // 9B01yKb6NB2K8+Ex4P9RHzkhg2OGCHiGhROFUaTwrT0Y
// SIG // 4gZR8JSYoF7FL4Xkyzg9i8gd/W0PFgHckssbqtEitVnm
// SIG // hGiYfjGvrDbOI1D6Ej0c0pbO0A==
// SIG // End signature block
