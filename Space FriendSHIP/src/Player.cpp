#include "Player.h"

Player::Player()
{

}

Player::~Player()
{

}

void Player::init(SDL_Renderer* renderer, string configFile, UpgradeManager* upgradeManager)
{
    m_healthBar = new HealthBar;
    inDash = false;

    m_configFile = "config\\" + configFile;
    fstream stream;
    string tmp;

    stream.open(m_configFile.c_str());

    stream >> tmp >> m_objectRect.w >> m_objectRect.h;
    stream >> tmp >> m_img;
    stream >> tmp >> m_imgShield;
    stream >> tmp >> m_maxhealth;
    stream >> tmp >> m_spawn.x >> m_spawn.y;
    stream >> tmp >> m_speed;
    stream >> tmp >> m_max_speed;
    stream >> tmp >> m_min_speed;
    stream >> tmp >> m_dashLenght;
    stream >> tmp >> m_rotationAngle;
    stream >> tmp >> s_move_left;
    stream >> tmp >> s_move_right;
    stream >> tmp >> s_gas;
    stream >> tmp >> s_brake;
    stream >> tmp >> s_dash;
    stream >> tmp >> s_shoot;
    stream >> tmp >> m_collisionDamage;
    stream >> tmp >> HPBar;
    stream >> tmp >> m_dashCooldown;
    stream >> tmp >> m_shootCooldown;

    stream.close();

    m_center.x = 40;
    m_center.y = 0;

    m_maxhealth += upgradeManager->m_CurrentHealthUpgrade;
    m_dashLenght += upgradeManager->m_CurrentDashUpgrade;

    if(s_gas == "W")
    {
        gas = SDL_SCANCODE_W;
    }
    if(s_brake == "S")
    {
        brake = SDL_SCANCODE_S;
    }
    if(s_move_left == "A")
    {
        move_left = SDL_SCANCODE_A;
    }
    if(s_move_right == "D")
    {
        move_right = SDL_SCANCODE_D;
    }
    if(s_dash == "E")
    {
        dash = SDL_SCANCODE_E;
    }
    if(s_shoot == "Q")
    {
        shoot = SDL_SCANCODE_Q;
    }

    if(s_gas == "Up")
    {
        gas = SDL_SCANCODE_UP;
    }
    if(s_brake == "Down")
    {
        brake = SDL_SCANCODE_DOWN;
    }
    if(s_move_left == "Left")
    {
        move_left = SDL_SCANCODE_LEFT;
    }
    if(s_move_right == "Right")
    {
        move_right = SDL_SCANCODE_RIGHT;
    }
    if(s_dash == "Right_Alt")
    {
        dash = SDL_SCANCODE_RALT;
    }
    if(s_shoot == "Right_Shift")
    {
        shoot = SDL_SCANCODE_RSHIFT;
    }

    Gun* gun = new Gun;
    gun -> init(100);
    m_guns.push_back(gun);

    m_elapsed_engage = chrono::high_resolution_clock::now();
    m_engagementRate = chrono::milliseconds(m_shootCooldown);

    m_img = "img\\" + m_img;
    m_imgShield = "img\\" + m_imgShield;

    m_objectRect.x = m_spawn.x;
    m_objectRect.y = m_spawn.y;
    m_coor.x = m_objectRect.x;
    m_coor.y = m_objectRect.y;

    m_health = m_maxhealth;
    m_healthBar->init(HPBar);

    SDL_Surface* loadingSurface = SDL_LoadBMP(m_img.c_str());
    SDL_Surface* loadingSurface1 = SDL_LoadBMP(m_imgShield.c_str());

    m_objectTexture = SDL_CreateTextureFromSurface(renderer, loadingSurface);
    m_TextureWithShield = SDL_CreateTextureFromSurface(renderer, loadingSurface1);

    SDL_FreeSurface(loadingSurface);
    SDL_FreeSurface(loadingSurface1);
}

void Player::update()
{
    checkforShooting();

    m_healthBar -> update(m_health, m_maxhealth);
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    m_screen_speed = m_speed  * SPEED_FACTOR;

    for(int i = 0; i < m_guns.size() ; i++)
    {
        coordinates playerCoor;
        playerCoor.x = m_objectRect.x;
        playerCoor.y = m_objectRect.y;
        m_guns[i] -> update(m_rotationAngle, findCenter(m_objectRect, m_rotationAngle, &m_center));
        m_guns[i] -> m_cantShoot = true;
    }

    if(state != NULL)
    {
        if(state[shoot] && m_canShoot)
        {
            for (int i = 0; i < m_guns.size(); i++)
            {
                m_guns[i] -> m_cantShoot = false;
            }
            m_elapsed_engage = chrono::high_resolution_clock::now();
            m_canShoot = false;
        }
        if (state[gas])
        {
            m_speed += 0.7;

            if(m_speed > m_max_speed)
            {
                m_speed = m_max_speed;
            }
        }
        else if(state[brake])
        {
            m_speed -=0.5;

            if(m_speed < m_min_speed)
            {
                m_speed = m_min_speed;
            }
        }
        else if(state[move_right])
        {
            m_rotationAngle += 5;
        }
        else if(state[move_left])
        {
            m_rotationAngle -= 5;
        }
    }

    m_coor.x += sin(m_rotationAngle * PI / 180) * m_screen_speed;
    m_coor.y -= cos(m_rotationAngle * PI / 180) * m_screen_speed;

    checkForDash();

    if(m_canDash)
    {
        inDash = true;
        m_oldCoor.x = m_objectRect.x;
        m_oldCoor.y = m_objectRect.y;

        m_coor.x += sin(m_rotationAngle * PI / 180) * m_dashLenght;
        m_coor.y -= cos(m_rotationAngle * PI / 180) * m_dashLenght;
    }

    if(m_health > m_maxhealth)
    {
        m_health = m_maxhealth;
    }

    m_objectRect.x = m_coor.x;
    m_objectRect.y = m_coor.y;
}

void Player::draw(SDL_Renderer* renderer)
{
    if(inShield)
    {
        SDL_RenderCopyEx(renderer, m_TextureWithShield, NULL, &m_objectRect, m_rotationAngle, &m_center, SDL_FLIP_NONE);
    }
    else
    {
        SDL_RenderCopyEx(renderer, m_objectTexture, NULL, &m_objectRect, m_rotationAngle, &m_center, SDL_FLIP_NONE);
    }

    m_healthBar -> draw(renderer);
}

void Player::checkForDash()
{
    const Uint8 *state = SDL_GetKeyboardState(NULL);

    if(m_startDashCooldown + m_dashCooldown < time(NULL))
    {
        m_hasCooldown = false;
    }
    if(state[dash] && !m_hasCooldown)
    {
        m_hasCooldown = true;
        m_startDashCooldown = time(NULL);
        m_canDash = true;
    }
    else
    {
        m_canDash = false;
    }
}

bool Player::checkforShooting()
{
    if(chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - m_elapsed_engage) > m_engagementRate)
    {
        m_canShoot = true;
    }
}
