int NearestBoneToCrosshair(CClientEntityList* target)
{
    int best;
    float maxfov = FLT_MAX;
    const int size = 4;
    int bones[size] = { 0,1,2,3 };
    Vector3 FeetPosition = target->Origin();
    for (int num = 0; num < size; num++) {
        if (target != 0) {
            float fov = GetFov(LocalPlayer->ViewAngles(), CalcAngles(LocalPlayer->CameraPos(), target->HitBoxPos(bones[num])));
            if (fov < maxfov) {
                maxfov = fov;
                best = bones[num];
            }
        }
    }
    return best;
}

void SaveEntityCache()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::vector<CClientEntityList*> Players = std::vector<CClientEntityList*>();

        int count = Configs::FiringRangeMode ? 15000 : 64;
        int LocalTeam = LocalPlayer->TeamID();
        Vector3 LocalOrigin = LocalPlayer->Origin();

        for (int i = 0; i <= count; i++)
        {
            CClientEntityList* CurEnt = GetEntity(i);
            if (CurEnt == LocalPlayer || !CurEnt) continue;

  

            if (CurEnt->TeamID() == LocalTeam && Configs::EnableTeam) continue;

            if (CurEnt->IsPlayer() || (CurEnt->IsDummie() && Configs::FiringRangeMode))
                Players.push_back(CurEnt);

        }

        PlayerCache = Players;
        Players.clear();
    }
}



void CombatThread()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (!LocalPlayer->IsValidPlayer()) continue;

        if (!Configs::Aimbot) continue;



        bool aimKey = 0;
        switch (Configs::Aimkey) {
        case 0: // "Attack"
            aimKey = Attacking();
            break;
        case 1: // "Zoom"
            aimKey = Zooming();
            break;
        case 2:
            aimKey = Attacking() || Zooming();
            break;
        }



        Vector3 Bone = { 0,0,0 };

        if (Target && aimKey && Configs::Aimbot)
        {
            Vector3 ViewAngles = LocalPlayer->ViewAngles();
            Vector3 SwayAngles = LocalPlayer->SwayAngles();
            Vector3 Camera = LocalPlayer->CameraPos();
            Vector3 Punch = LocalPlayer->PunchAngles() * 0.05;

            if (Configs::BoneID == 3)
                Bone = Target->HitBoxPos(NearestBoneToCrosshair(Target));
            else
                Bone = Target->HitBoxPos(Configs::BoneID);

            LocalPlayer->PredictPos(Target, &Bone);

            Vector3 newAngles = CalcAngles(Camera, Bone);

            newAngles -= Punch;
            newAngles -= SwayAngles;
            newAngles += ViewAngles;
            newAngles.Normalize();

            Vector3 Delta = (newAngles - ViewAngles);
            Delta.Normalize();

            if (Delta.x > 0.0f) {
                Delta.x /= Configs::Smooth;
            }
            else {
                Delta.x = ((Delta.x * -1L) / Configs::Smooth) * -1.f;
            }

            if (Delta.y > 0.0f) {
                Delta.y /= Configs::Smooth;
            }
            else {
                Delta.y = ((Delta.y * -1L) / Configs::Smooth) * -1.f;
            }

            Vector3 SmoothedAngles = Delta + ViewAngles;
            SmoothedAngles.Normalize();


            if (ViewAngles.x != SmoothedAngles.x || ViewAngles.y != SmoothedAngles.y)
            {
                LocalPlayer->SetViewAngles(SmoothedAngles);
            }
        }




    }
}

void ItemGlowThread()
{

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!LocalPlayer->IsValidPlayer()) continue;

        for (int i = 0; i < 15000; i++)
        {
            CClientEntityList* CurEnt = GetEntity(i);
            if (CurEnt == LocalPlayer || !CurEnt) continue;

   
            if (!CurEnt->IsItem()) continue;

            if (Configs::ItemGlow)
                Driver.Write<int>(CurEnt + Offsets::m_highlightFunctionBits, 1396738697);
            else
                Driver.Write<int>(CurEnt + Offsets::m_highlightFunctionBits, 1411417991);
        }
        
    }
}

void GlowThread()
{

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));

        if (LocalPlayer->IsOnGround() && LocalPlayer->IsValidPlayer() && Configs::GlowESP)
        {
            for (int i = 0; i <  PlayerCache.size(); i++)
            {
                CClientEntityList* CurEnt = PlayerCache[i];

                float health = CurEnt->Health();

                if (health <= 0) continue;

                static ImVec4 gcolor;


                Driver.Write<int>(CurEnt + GLOW_CONTEXT, 1);

                Driver.Write<int>(CurEnt + GLOW_VISIBLE_TYPE, 1);

                Driver.Write<glowMode>(CurEnt + GLOW_TYPE, { 101, 118, 46, 96 });

                Driver.Write<Vector3>(CurEnt + GLOW_COLOR, { 1,1,1 });
            }
        }
    }
}




void Main()
{
    SetConsoleTitleA("");

    Driver.Init();

    printf(xorstr_("[<] Waiting Game...\n"));

    while (!GameWindow)
        GameWindow = FindWindowA(xorstr_("Respawn001"), 0);

    printf(xorstr_("[+] hWnd = 0x%x\n"), GameWindow);

    while (!m_Process)
        m_Process = Driver.GetProcID(xorstr_(L"r5apex.exe"));

    printf(xorstr_("[+] m_Process = %d\n"), m_Process);

    Driver.SetProcess(m_Process);

    while (!m_GameImage)
        m_GameImage = Driver.GetBaseAddress();

    printf(xorstr_("[+] m_GameImage = 0x%llx\n"), m_GameImage);


    std::thread(SaveEntityCache).detach();
    std::thread(CombatThread).detach();
    std::thread(GlowThread).detach();
    std::thread(ItemGlowThread).detach();


    printf("[+] Finished!\n");
    
    while (!GetAsyncKeyState(VK_END))
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        LocalPlayer = GetLocalPlayer();
        Target = GetBestTarget();
   
        char szName[64] = { 0 };
        Driver.ReadI(m_GameImage + Offsets::LevelName, szName, sizeof(szName));

        Configs::FiringRangeMode = strstr(szName, xorstr_("mp_rr_canyonlands_staging_mu1"));

    }

    exit(1);
}