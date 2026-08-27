#define MyAppName "VOIDWORM"
#define MyAppVersion "1.0.0"
#define MyAppFileVersion "1.0.0.0"
#define MyAppPublisher "LWNX DSP"
#define MyAppCopyright "© 2026 lewonn / LWNX DSP"
#define MyAppExeName "VOIDWORM.exe"

[Setup]
AppId={{D92298E2-3D5A-4F24-95D7-2A615C27C50E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
VersionInfoVersion={#MyAppFileVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoCopyright={#MyAppCopyright}
VersionInfoDescription={#MyAppName}
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppFileVersion}
DefaultDirName={autopf}\LWNX DSP\VOIDWORM
DefaultGroupName=LWNX DSP\VOIDWORM
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=..\dist\release
OutputBaseFilename=VOIDWORM-1.0.0-Windows-x64-Setup
SetupIconFile=assets\VOIDWORM_Installer_Icon.ico
UninstallDisplayIcon={app}\VOIDWORM.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern dark includetitlebar hidebevels
WizardBackColor=#09070D
WizardImageFile=
WizardSmallImageFile=
WizardSizePercent=120,110
ChangesAssociations=no
CloseApplications=yes

[Messages]
SetupWindowTitle=VOIDWORM
WelcomeLabel1=VOIDWORM

[Types]
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst3"; Description: "VST3 plug-in"; Types: custom
Name: "standalone"; Description: "Standalone application"; Types: custom

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional shortcuts:"; Components: standalone; Flags: checkedonce

[Files]
Source: "..\VOIDWORM_Master_Artwork_512.png"; Flags: dontcopy
Source: "assets\VOIDWORM_Installer_Icon.ico"; DestDir: "{app}"; DestName: "VOIDWORM.ico"; Flags: ignoreversion
Source: "..\dist\VOIDWORM.vst3\*"; DestDir: "{commoncf64}\VST3\VOIDWORM.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\dist\VOIDWORM.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\VOIDWORM"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Components: standalone
Name: "{autodesktop}\VOIDWORM"; Filename: "{app}\{#MyAppExeName}"; WorkingDir: "{app}"; Components: standalone; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch VOIDWORM"; Components: standalone; Flags: nowait postinstall skipifsilent

[Code]
var
  BrandPanel: TPanel;
  BrandBackground: TBitmapImage;
  BrandAccent: TPanel;
  BrandImage: TBitmapImage;
  BrandLabel: TNewStaticText;
  BrandPublisher: TNewStaticText;
  BrandAuthor: TNewStaticText;
  BrandVersion: TNewStaticText;
  BrandCopyright: TNewStaticText;
  TasksContainer: TPanel;

procedure DrawSidebarGradient;
var
  Y: Integer;
  H: Integer;
  R: Integer;
  G: Integer;
  B: Integer;
begin
  BrandBackground.Bitmap.Width := BrandBackground.Width;
  BrandBackground.Bitmap.Height := BrandBackground.Height;
  H := BrandBackground.Height - 1;
  if H < 1 then
    H := 1;

  for Y := 0 to BrandBackground.Height - 1 do
  begin
    R := 13 - (5 * Y div H);
    G := 11 - (4 * Y div H);
    B := 18 - (6 * Y div H);
    BrandBackground.Bitmap.Canvas.Pen.Color := R or (G shl 8) or (B shl 16);
    BrandBackground.Bitmap.Canvas.MoveTo(0, Y);
    BrandBackground.Bitmap.Canvas.LineTo(BrandBackground.Width, Y);
  end;
end;

procedure InitializeWizard;
var
  SidebarWidth: Integer;
  ContentWidth: Integer;
  OuterLeft: Integer;
  OuterTop: Integer;
  OuterWidth: Integer;
  OuterHeight: Integer;
  BevelLeft: Integer;
  BevelTop: Integer;
  BevelWidth: Integer;
  BevelHeight: Integer;
  BackLeft: Integer;
  NextLeft: Integer;
  CancelLeft: Integer;
  BeveledLabelLeft: Integer;
  ArtSize: Integer;
begin
  SidebarWidth := ScaleX(142);
  ContentWidth := WizardForm.ClientWidth;

  OuterLeft := WizardForm.OuterNotebook.Left;
  OuterTop := WizardForm.OuterNotebook.Top;
  OuterWidth := WizardForm.OuterNotebook.Width;
  OuterHeight := WizardForm.OuterNotebook.Height;
  BevelLeft := WizardForm.Bevel.Left;
  BevelTop := WizardForm.Bevel.Top;
  BevelWidth := WizardForm.Bevel.Width;
  BevelHeight := WizardForm.Bevel.Height;
  BackLeft := WizardForm.BackButton.Left;
  NextLeft := WizardForm.NextButton.Left;
  CancelLeft := WizardForm.CancelButton.Left;
  BeveledLabelLeft := WizardForm.BeveledLabel.Left;

  WizardForm.ClientWidth := ContentWidth + SidebarWidth;
  WizardForm.OuterNotebook.SetBounds(
    OuterLeft + SidebarWidth, OuterTop, OuterWidth, OuterHeight);
  WizardForm.Bevel.SetBounds(
    BevelLeft + SidebarWidth, BevelTop, BevelWidth, BevelHeight);
  WizardForm.BackButton.Left := BackLeft + SidebarWidth;
  WizardForm.NextButton.Left := NextLeft + SidebarWidth;
  WizardForm.CancelButton.Left := CancelLeft + SidebarWidth;
  WizardForm.BeveledLabel.Left := BeveledLabelLeft + SidebarWidth;

  BrandPanel := TPanel.Create(WizardForm);
  BrandPanel.Parent := WizardForm;
  BrandPanel.SetBounds(0, 0, SidebarWidth, WizardForm.ClientHeight);
  BrandPanel.Anchors := [akLeft, akTop, akBottom];
  BrandPanel.BevelOuter := bvNone;
  BrandPanel.Caption := '';
  BrandPanel.Color := StrToColor('#0B0910');
  BrandPanel.StyleElements := [];

  BrandBackground := TBitmapImage.Create(BrandPanel);
  BrandBackground.Parent := BrandPanel;
  BrandBackground.SetBounds(0, 0, SidebarWidth, BrandPanel.Height);
  BrandBackground.Anchors := [akLeft, akTop, akRight, akBottom];
  BrandBackground.BackColor := BrandPanel.Color;
  BrandBackground.Stretch := False;
  DrawSidebarGradient;

  BrandAccent := TPanel.Create(BrandPanel);
  BrandAccent.Parent := BrandPanel;
  BrandAccent.Align := alRight;
  BrandAccent.Width := ScaleX(2);
  BrandAccent.BevelOuter := bvNone;
  BrandAccent.Caption := '';
  BrandAccent.Color := StrToColor('#52008A');
  BrandAccent.StyleElements := [];

  ExtractTemporaryFile('VOIDWORM_Master_Artwork_512.png');
  ArtSize := SidebarWidth - ScaleX(34);
  BrandImage := TBitmapImage.Create(BrandPanel);
  BrandImage.Parent := BrandPanel;
  BrandImage.SetBounds(
    (SidebarWidth - ArtSize) div 2, ScaleY(48), ArtSize, ArtSize);
  BrandImage.BackColor := clNone;
  BrandImage.Center := True;
  BrandImage.Stretch := True;
  BrandImage.PngImage.LoadFromFile(
    ExpandConstant('{tmp}\VOIDWORM_Master_Artwork_512.png'));

  BrandLabel := TNewStaticText.Create(BrandPanel);
  BrandLabel.Parent := BrandPanel;
  BrandLabel.SetBounds(
    ScaleX(10), BrandImage.Top + BrandImage.Height + ScaleY(20),
    SidebarWidth - ScaleX(22), ScaleY(24));
  BrandLabel.AutoSize := False;
  BrandLabel.Alignment := taCenter;
  BrandLabel.Caption := 'VOIDWORM';
  BrandLabel.Color := BrandPanel.Color;
  BrandLabel.Font.Name := 'Bahnschrift';
  BrandLabel.Font.Size := 12;
  BrandLabel.Font.Style := [fsBold];
  BrandLabel.Font.Color := StrToColor('#E6E1EA');
  BrandLabel.StyleElements := [];

  BrandPublisher := TNewStaticText.Create(BrandPanel);
  BrandPublisher.Parent := BrandPanel;
  BrandPublisher.SetBounds(
    ScaleX(10), BrandLabel.Top + BrandLabel.Height + ScaleY(12),
    SidebarWidth - ScaleX(22), ScaleY(18));
  BrandPublisher.AutoSize := False;
  BrandPublisher.Alignment := taCenter;
  BrandPublisher.Caption := 'LWNX DSP';
  BrandPublisher.Color := BrandPanel.Color;
  BrandPublisher.Font.Name := 'Bahnschrift';
  BrandPublisher.Font.Size := 9;
  BrandPublisher.Font.Style := [fsBold];
  BrandPublisher.Font.Color := StrToColor('#B785DF');
  BrandPublisher.StyleElements := [];

  BrandAuthor := TNewStaticText.Create(BrandPanel);
  BrandAuthor.Parent := BrandPanel;
  BrandAuthor.SetBounds(
    ScaleX(10), BrandPublisher.Top + BrandPublisher.Height + ScaleY(3),
    SidebarWidth - ScaleX(22), ScaleY(16));
  BrandAuthor.AutoSize := False;
  BrandAuthor.Alignment := taCenter;
  BrandAuthor.Caption := 'lewonn';
  BrandAuthor.Color := BrandPanel.Color;
  BrandAuthor.Font.Name := 'Bahnschrift';
  BrandAuthor.Font.Size := 8;
  BrandAuthor.Font.Color := StrToColor('#AAA3AF');
  BrandAuthor.StyleElements := [];

  BrandVersion := TNewStaticText.Create(BrandPanel);
  BrandVersion.Parent := BrandPanel;
  BrandVersion.SetBounds(
    ScaleX(10), BrandAuthor.Top + BrandAuthor.Height + ScaleY(2),
    SidebarWidth - ScaleX(22), ScaleY(16));
  BrandVersion.AutoSize := False;
  BrandVersion.Alignment := taCenter;
  BrandVersion.Caption := 'Version 1.0.0';
  BrandVersion.Color := BrandPanel.Color;
  BrandVersion.Font.Name := 'Bahnschrift';
  BrandVersion.Font.Size := 8;
  BrandVersion.Font.Color := StrToColor('#928B99');
  BrandVersion.StyleElements := [];

  BrandCopyright := TNewStaticText.Create(BrandPanel);
  BrandCopyright.Parent := BrandPanel;
  BrandCopyright.SetBounds(
    ScaleX(5), BrandVersion.Top + BrandVersion.Height + ScaleY(7),
    SidebarWidth - ScaleX(12), ScaleY(16));
  BrandCopyright.AutoSize := False;
  BrandCopyright.Alignment := taCenter;
  BrandCopyright.Caption := '© 2026 lewonn / LWNX DSP';
  BrandCopyright.Color := BrandPanel.Color;
  BrandCopyright.Font.Name := 'Bahnschrift Condensed';
  BrandCopyright.Font.Size := 7;
  BrandCopyright.Font.Color := StrToColor('#7F7886');
  BrandCopyright.StyleElements := [];

  WizardForm.WizardBitmapImage.Visible := False;
  WizardForm.WizardBitmapImage2.Visible := False;
  WizardForm.WizardSmallBitmapImage.Visible := False;

  WizardForm.WelcomeLabel1.Left := ScaleX(24);
  WizardForm.WelcomeLabel1.Width := OuterWidth - ScaleX(48);
  WizardForm.WelcomeLabel2.Left := ScaleX(24);
  WizardForm.WelcomeLabel2.Width := OuterWidth - ScaleX(48);
  WizardForm.FinishedHeadingLabel.Left := ScaleX(24);
  WizardForm.FinishedHeadingLabel.Width := OuterWidth - ScaleX(48);
  WizardForm.FinishedLabel.Left := ScaleX(24);
  WizardForm.FinishedLabel.Width := OuterWidth - ScaleX(48);

  WizardForm.PageNameLabel.Width :=
    WizardForm.MainPanel.Width - WizardForm.PageNameLabel.Left - ScaleX(24);
  WizardForm.PageDescriptionLabel.Width :=
    WizardForm.MainPanel.Width - WizardForm.PageDescriptionLabel.Left - ScaleX(24);

  if WizardForm.TypesCombo.Visible then
  begin
    WizardForm.ComponentsList.Height := WizardForm.ComponentsList.Height +
      (WizardForm.ComponentsList.Top - WizardForm.TypesCombo.Top);
    WizardForm.ComponentsList.Top := WizardForm.TypesCombo.Top;
    WizardForm.TypesCombo.Visible := False;
  end;

  WizardForm.DirEdit.Color := StrToColor('#100D14');
  WizardForm.DirEdit.Font.Color := StrToColor('#E0DAE4');
  WizardForm.DirEdit.StyleElements := [seBorder];
  WizardForm.ComponentsList.Color := StrToColor('#100D14');
  WizardForm.ComponentsList.Font.Color := StrToColor('#D8D2DD');
  WizardForm.ComponentsList.BorderStyle := bsNone;
  WizardForm.ComponentsList.StyleElements := [];
  WizardForm.TasksList.Color := StrToColor('#100D14');
  WizardForm.TasksList.Font.Color := StrToColor('#D8D2DD');
  WizardForm.TasksList.StyleElements := [seBorder];
  WizardForm.TasksList.Top := WizardForm.TasksList.Top + ScaleY(16);
  WizardForm.TasksList.Height := ScaleY(102);

  TasksContainer := TPanel.Create(WizardForm.SelectTasksPage);
  TasksContainer.Parent := WizardForm.SelectTasksPage;
  TasksContainer.SetBounds(
    WizardForm.TasksList.Left,
    WizardForm.TasksList.Top,
    WizardForm.TasksList.Width,
    WizardForm.TasksList.Height);
  TasksContainer.BevelOuter := bvNone;
  TasksContainer.BevelInner := bvNone;
  TasksContainer.BorderStyle := bsNone;
  TasksContainer.Caption := '';
  TasksContainer.Color := StrToColor('#100D14');
  TasksContainer.StyleElements := [];

  WizardForm.TasksList.Parent := TasksContainer;
  WizardForm.TasksList.SetBounds(
    ScaleX(10), ScaleY(12),
    TasksContainer.ClientWidth - ScaleX(20),
    TasksContainer.ClientHeight - ScaleY(24));
  WizardForm.TasksList.BorderStyle := bsNone;
  WizardForm.TasksList.Anchors := [akLeft, akTop, akRight, akBottom];
  WizardForm.ReadyMemo.Color := StrToColor('#100D14');
  WizardForm.ReadyMemo.Font.Color := StrToColor('#D8D2DD');
  WizardForm.ReadyMemo.StyleElements := [seBorder];
  WizardForm.PreparingMemo.Color := StrToColor('#100D14');
  WizardForm.PreparingMemo.Font.Color := StrToColor('#D8D2DD');
  WizardForm.PreparingMemo.StyleElements := [seBorder];
  BrandPanel.BringToFront;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  if BrandPanel <> nil then
    BrandPanel.BringToFront;
end;
