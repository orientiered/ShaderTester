#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <cmath>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <Windows.h>
#include <shobjidl_core.h>
#include <fstream>
#include <thread>
using namespace std;
using namespace sf;

Font font;

Vector2f reverseY(Vector2f vec) {
	vec.y = -vec.y;
	return vec;
}

template <typename T>
Vector2<T> operator* (const Vector2<T> left, const Vector2<T> right) {
	return Vector2<T>(left.x * right.x, left.y * right.y);
}

Vector2f operator/ (const Vector2f left, const Vector2f right) { 
	return Vector2f(left.x / right.x, left.y / right.y);
}


vector<char> loadFile(wstring path) {
	std::ifstream texture_file(path, std::ifstream::binary);
	std::vector<char> buffer;
	if (texture_file) {
		// get length of file:
		texture_file.seekg(0, texture_file.end);
		const auto length = texture_file.tellg();
		if (!length) {
			std::cerr << "Cannot load zero byte texture file" << std::endl;
		}
		buffer.resize(length); // reserve space

		texture_file.seekg(0, texture_file.beg);

		auto start = &*buffer.begin();
		texture_file.read(start, length);
		texture_file.close();
	}
	else {
		std::cerr << "Could not open texture file" << std::endl;
	}
	return buffer;

}

wstring browseFile() {
	wstring filePath = L"";
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	if (SUCCEEDED(hr)) {
		IFileDialog* pFileDialog;
		hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileDialog, reinterpret_cast<void**>(&pFileDialog));
		if (SUCCEEDED(hr)) {
			DWORD dwOptions;
			hr = pFileDialog->GetOptions(&dwOptions);
			if (SUCCEEDED(hr)) {
				hr = pFileDialog->Show(NULL);
				if (SUCCEEDED(hr)) {
					IShellItem* pShellItem;
					hr = pFileDialog->GetResult(&pShellItem);
					if (SUCCEEDED(hr)) {
						PWSTR pszFilePath;
						hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
						if (SUCCEEDED(hr)) {
							filePath = pszFilePath;
							CoTaskMemFree(pszFilePath);
						}
						pShellItem->Release();
					}
				}
			}
			pFileDialog->Release();
		}
		CoUninitialize();
	}
	return filePath;
}

class Button {
private:
	Texture tex;
	Sprite icon;
	bool state = 0, click = 0;
	RenderWindow& window;
public:
	int id = -1;

	Button(Vector2f pos, Vector2f size, RenderWindow& w, const wchar_t* texture_path) : window(w) {

		auto buffer = loadFile(texture_path);
		tex.loadFromMemory(&buffer[0], buffer.size());
		tex.setSmooth(1);
		icon.setTexture(tex);
		icon.setScale(size / Vector2f(tex.getSize()));
		icon.setPosition(pos);

	}

	void move(Vector2f delta) {
		icon.move(delta);
	}
	bool handle() {
		click = 0;
		if (icon.getGlobalBounds().contains(Vector2f(Mouse::getPosition(window)))) {
			icon.setColor(Color(227, 212, 188));
			if (state && !Mouse::isButtonPressed(Mouse::Left)) {
				state = 0;
				click = 1;
			}
			if (!state && Mouse::isButtonPressed(Mouse::Left)) state = 1;
		}
		else {
			icon.setColor(Color(245, 237, 225));
			state = 0;

		}
		return click;
	}
	void draw() {
		window.draw(icon);
	}

};

class Option {
private:
	RectangleShape background;
	Text fileName;
	Button* deleteShader;
	RenderWindow& window;
	Shader shader;
public:
	void asyncLoad() {
		wstring text;
		wstring path = browseFile();
		vector<char> buff = loadFile(path);


		text = path.substr(path.rfind(L"\\") + 1);
		if (!shader.loadFromMemory(string(buff.begin(), buff.end()), Shader::Fragment)) {
			text = L"error";
			shader.loadFromFile("res/default.frag", Shader::Fragment);
		}
		fileName.setString(text);
	}
	Option(Vector2f pos, Vector2f size, RenderWindow& wnd) : window(wnd) {
		background.setSize(size);
		background.setPosition(pos);
		background.setFillColor(Color(95, 95, 95, 200));
		wstring text = L"Loading...";

		fileName.setFont(font);
		fileName.setString(text);
		fileName.setPosition(pos + size / 10.f);

		shader.loadFromFile("res/default.frag", Shader::Fragment);

		deleteShader = new Button(pos + size * Vector2f(0.8, 0.1), Vector2f(size.y * 0.8, size.y * 0.8), window, L"res/delete.png");

		thread fileLoader(&Option::asyncLoad, this);
		fileLoader.detach();
	}
	Shader* getShader() {
		return &shader;
	}
	~Option() {
		delete deleteShader;
	}

	void move(Vector2f delta) {
		background.move(delta);
		fileName.move(delta);
		deleteShader->move(delta);
	}
	bool handle() {
		return deleteShader->handle();
	}
	void draw() {
		window.draw(background);
		window.draw(fileName);
		deleteShader->draw();
	}

};

class RectangleShader {
private:
	RectangleShape rect;
	RectangleShape titleBar;
	RectangleShape resizeIcon1, resizeIcon2;

	RenderWindow& window;
	Clock& clock;

	Vector2f offset = { 0., 0. };
	float scale = 1;
	enum class MoveType { None, Shader, Window, Resize };
	MoveType mType = MoveType::None;



public:
	Option* opt;

	Vector2f iMouse;
	bool smode = false; //доп. переменная; инвертируется при нажатии колёсика мыши
	
	RectangleShader(Vector2f size, Vector2f pos, Clock& clk, RenderWindow& wind, Option* ptr) : window(wind), clock(clk), opt(ptr) {
		rect.setSize(size);
		rect.setPosition(pos);

		titleBar.setSize(Vector2f(size.x, 10));
		titleBar.setPosition(pos);
		titleBar.setFillColor(Color::White);


		resizeIcon1.setFillColor(Color::White);
		resizeIcon1.setSize({ 20, 5 });
		resizeIcon1.setPosition(pos + size + Vector2f(-18, -3));

		resizeIcon2.setFillColor(Color::White);
		resizeIcon2.setSize({ 5, 20 });
		resizeIcon2.setPosition(pos + size + Vector2f(-3, -18));

		offset = -Vector2f(rect.getPosition().x, float(window.getSize().y) - float(rect.getSize().y) - rect.getPosition().y);
	}
	Vector2f getCenter() {
		return Vector2f(-offset.x + rect.getSize().x / 2, window.getSize().y - rect.getSize().y / 2 + offset.y);
	}
	Vector2f getPosition() { return rect.getPosition(); }
	Vector2f getSize() { return rect.getSize(); }
	FloatRect getBoundary() { return rect.getGlobalBounds(); }

	void iMouseUpdate() {
		iMouse = Vector2f(Mouse::getPosition(window));
	}
	void zoom(float delta) {
		delta = delta > 0 ? 1.f : -1.f;
		float k = pow(0.7, -delta);
		if (scale * k > 0.1) {
			scale *= k;
			offset += reverseY(Vector2f(Mouse::getPosition(window)) - (getCenter())) * (k - 1);
		}
	}
	void setMoveType(Vector2i posi) {
		Vector2f pos = Vector2f(posi);
		mType = MoveType::None;
		if (FloatRect(rect.getPosition(), rect.getSize()).contains(pos))
			mType = MoveType::Shader;
		if (FloatRect(rect.getPosition().x, rect.getPosition().y, rect.getSize().x, 10.0f).contains(pos))
			mType = MoveType::Window;
		if (FloatRect(rect.getPosition().x + rect.getSize().x - 10., rect.getPosition().y + rect.getSize().y - 10., 20., 20.).contains(pos))
			mType = MoveType::Resize;
	}
	void move(Vector2i lastPos) {
		Vector2f delta = Vector2f(Mouse::getPosition(window) - lastPos);
		switch (mType) {
		case MoveType::Shader:
			offset += -reverseY(delta);
			break;
		case MoveType::Window:
			rect.move(delta);
			titleBar.move(delta);
			resizeIcon1.move(delta);
			resizeIcon2.move(delta);
			offset += -reverseY(delta);
			break;
		case MoveType::Resize:
			scale /= 1 + delta.y / rect.getSize().y;
			rect.setSize(rect.getSize() + delta);
			titleBar.setSize({ rect.getSize().x, 10 });
			resizeIcon1.move(delta);
			resizeIcon2.move(delta);
			offset += delta / 2.f;
			break;
		}

	}
	void draw() {
		Shader* shader = opt->getShader();
		shader->setUniform("iResolution", Vector2f(rect.getSize()));
		shader->setUniform("windowRes", Vector2f(window.getSize()));
		shader->setUniform("iTime", clock.getElapsedTime().asSeconds());
		shader->setUniform("offset", offset);
		shader->setUniform("globalScale", scale);
		shader->setUniform("iMouse", Glsl::Vec3{iMouse.x, iMouse.y, float(smode)});
		window.draw(rect, shader);
		window.draw(titleBar);
		window.draw(resizeIcon1);
		window.draw(resizeIcon2);
	}

};

enum class Actions { LeftClick, LeftRelease, Drag, RDrag, WheelClick, Zoom };
struct updData {
	float zoom = 0;
	Vector2i pos;
};

class WindowManager {
private:
	Clock& clock;
	RenderWindow& window;

public:
	vector<RectangleShader*> shaderWindows;
	WindowManager(Clock& clk, RenderWindow& wnd) : clock(clk), window(wnd) {

	}
	unsigned getWindowIndex(Option* opt) {
		for (unsigned i = 0; i < shaderWindows.size(); ++i) {
			if (shaderWindows[i]->opt == opt) return i;
		}
	}
	void update(Actions type, updData data, FloatRect restricted = FloatRect()) {
		if (restricted.contains(Vector2f(Mouse::getPosition()))) return;

		for (int i = shaderWindows.size() - 1; i >= 0; --i) {
			RectangleShader* wnd = shaderWindows[i];

			if (i == shaderWindows.size() - 1 && type == Actions::Drag) {
				wnd->move(data.pos);
			}

			if (type == Actions::LeftRelease)
				wnd->setMoveType(data.pos);

			if (wnd->getBoundary().contains(Vector2f(Mouse::getPosition()))) {
				switch (type) {
				case Actions::WheelClick:
					wnd->smode = !wnd->smode;
					break;
				case Actions::Zoom:
					wnd->zoom(data.zoom);
					break;
				case Actions::LeftClick:
					wnd->setMoveType(data.pos);
					swap(shaderWindows[i], shaderWindows[shaderWindows.size() - 1]);
					break;
				/*case Actions::Drag:
					wnd->move(data.pos);
					break;*/
				case Actions::RDrag:
					wnd->iMouseUpdate();
					break;
				}
				break;
			}
		}
	}
	void draw() {
		for (auto* wnd : shaderWindows) wnd->draw();
	}
};

class SideMenu {
private:
	RectangleShape menuBlock;
	ConvexShape sideButton;
	RenderWindow& window;
	Clock& clock;
	bool closed = true;
	Button* addShader;
	vector<Option*> loadedShaders;
	WindowManager& wm;
public:
	SideMenu(RenderWindow& window, Clock& clk, WindowManager& wman) : window(window), clock(clk), wm(wman) {
		Vector2f size = Vector2f(window.getSize());
		menuBlock.setFillColor(Color(60, 60, 60, 150));
		menuBlock.setSize(size * Vector2f(0.25, 0.75));
		menuBlock.setPosition(size * Vector2f(-0.25, 0.1));

		sideButton.setPointCount(4);
		sideButton.setPoint(0, size * Vector2f(0, 0));
		sideButton.setPoint(1, size * Vector2f(0, 0.15));
		sideButton.setPoint(2, size * Vector2f(0.01, 0.12));
		sideButton.setPoint(3, size * Vector2f(0.01, 0.03));
		sideButton.setPosition({ 0, size.y * 0.25f });
		sideButton.setFillColor(Color::Cyan);

		addShader = new Button(size * Vector2f(-0.15, 0.75), Vector2f(size.y*0.05, size.y*0.05), window, L"res/add.png");
	}
	~SideMenu() {
		delete addShader;
	}
	void update() {
		if (sideButton.getGlobalBounds().contains(Vector2f(Mouse::getPosition(window)))) {
			Vector2f delta = { window.getSize().x * 0.25f, 0 };
			if (!closed) delta = -delta;
			closed = !closed;
			menuBlock.move(delta);
			sideButton.move(delta);
			addShader->move(delta);
			for (auto* opt : loadedShaders)
				opt->move(delta);
		}
	}
	void addOption() {
		if (loadedShaders.size() >= 7) return;
		loadedShaders.push_back(nullptr);
		int index = loadedShaders.size() - 1;
		loadedShaders[index] = new Option(menuBlock.getPosition() + menuBlock.getSize() * Vector2f(0.1, 0.05 + 0.1 * index),
			menuBlock.getSize() * Vector2f(0.8, 0.06),  window);
		wm.shaderWindows.push_back(nullptr);
		wm.shaderWindows[index] = new RectangleShader(Vector2f(window.getSize()) / 2.f, Vector2f(window.getSize()) / 4.f, clock, window, loadedShaders[index]);
	}
	void deleteOption(int i) {
		if (i < 0) return;
		int j = wm.getWindowIndex(loadedShaders[i]);
		delete loadedShaders[i];
		delete wm.shaderWindows[j];
		loadedShaders.erase(loadedShaders.begin() + i);
		wm.shaderWindows.erase(wm.shaderWindows.begin() + j);
		for (int k = i; k < loadedShaders.size(); ++k) {
			loadedShaders[k]->move(menuBlock.getSize() * Vector2f(0, -0.1));
		}
	}
	FloatRect getBoundary() {
		return menuBlock.getGlobalBounds();
	}
	void draw() {
		if (addShader->handle()) {
			addOption();
		}
		int to_delete = -1;
		for (int i = 0; i < loadedShaders.size(); ++i) {
			if (loadedShaders[i]->handle()) to_delete = i;
		}
		deleteOption(to_delete);

		window.draw(menuBlock);
		window.draw(sideButton);
		addShader->draw();
		for (auto* opt : loadedShaders) {
			opt->draw();
		}
	}
};

class FPSmeter {
public:
	sf::Text FPS;
	sf::Clock clock;
	float lastTime = clock.getElapsedTime().asSeconds(), currentTime;
	FPSmeter(sf::Vector2f pos, int c) {
		FPS.setFont(font);
		FPS.setCharacterSize(c);
		FPS.setPosition(pos);
		FPS.setFillColor(sf::Color::White);
	}
	int draw(sf::RenderWindow& window) {
		currentTime = clock.getElapsedTime().asSeconds();
		int fps = floor(1.f / (currentTime - lastTime));
		lastTime = currentTime;
		FPS.setString(to_string(fps));
		window.draw(FPS);
		return fps;
	}
};

int main() {
	font.loadFromFile("res/Mulish.ttf");

	RenderWindow window(VideoMode::getDesktopMode(), "Shader works!", Style::None);
	window.setVerticalSyncEnabled(true);

	Clock clock;

	Vector2i lastPos;

	WindowManager wm(clock, window);
	SideMenu menu(window, clock, wm);
	FPSmeter fps(Vector2f(window.getSize())/100.f, 15);
	while (window.isOpen())
	{
		Event event;
		updData data;
		while (window.pollEvent(event))
		{
			switch (event.type) {
			case Event::Closed:
				window.close();
				break;
			case Event::KeyPressed:
				if (event.key.code == sf::Keyboard::Escape)
					window.close();
				break;
			case Event::MouseWheelScrolled:
				data.zoom = event.mouseWheelScroll.delta;
				wm.update(Actions::Zoom, data, menu.getBoundary());
				break;
			case Event::MouseButtonPressed:
				switch (event.mouseButton.button) {
				case Mouse::Left:
					lastPos = Mouse::getPosition(window);
					data.pos = lastPos;
					wm.update(Actions::LeftClick, data, menu.getBoundary());
					menu.update();
					break;
				case Mouse::Middle:
					wm.update(Actions::WheelClick, data, menu.getBoundary());
					break;
				}
				break;
			case Event::MouseButtonReleased:
				if (event.mouseButton.button == Mouse::Left)
					wm.update(Actions::LeftRelease, data);
			}
		}


		if (Mouse::isButtonPressed(Mouse::Left)) {
			data.pos = lastPos;
			wm.update(Actions::Drag, data);
			lastPos = Mouse::getPosition(window);

		}
		if (Mouse::isButtonPressed(Mouse::Right)) {
			wm.update(Actions::RDrag, data);
		}

		window.clear();
		wm.draw();
		menu.draw();
		fps.draw(window);
		window.display();
	}

	return 0;
}