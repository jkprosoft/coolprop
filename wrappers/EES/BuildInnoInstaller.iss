; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define MyAppName "CoolProp"
#define MyAppVersion "4.2"
#define MyAppPublisher "University of Liege"
#define MyAppURL "coolprop.sf.net"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{AECD4969-A5A5-4AF4-97AD-544A3890BF1C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName=c:\ees32\Userlib\COOLPROP_EES
DisableDirPage=yes
DefaultGroupName=CoolProp_EES
OutputBaseFilename=SetupCOOLPROP_EES
Compression=lzma
SolidCompression=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "C:\Users\Belli\Documents\Code\CoolProp\wrappers\EES\CoolProp.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Belli\Documents\Code\CoolProp\wrappers\EES\CoolProp.LIB"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Belli\Documents\Code\CoolProp\wrappers\EES\COOLPROP_EES.dlf"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Belli\Documents\Code\CoolProp\wrappers\EES\CoolProp_EES_Sample.EES"; DestDir: "{app}"; Flags: ignoreversion
Source: "C:\Users\Belli\Documents\Code\CoolProp\wrappers\EES\coolpropsi.LIB"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

